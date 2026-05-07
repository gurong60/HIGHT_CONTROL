/**
 * @file params.c - 参数持久化存储驱动（延时30秒保存至Flash）
 * @brief 将LED亮度、脉冲宽度、工作模式等参数存储在STM32内部Flash中
 *
 * 功能说明：
 *  与常规直接保存不同：每次参数修改（按键/串口）后，延时30秒再执行Flash写入
 *  多次快速修改会合并为1次写入，大幅降低Flash擦写次数，延长Flash使用寿命
 *
 * 硬件配置：
 *  STM32G030C8T6：64KB Flash，每页大小2KB
 *  参数存储位置：最后一页(页31)，地址0x0800F800
 *  Flash寿命：约10000次擦写 → 延时保存机制可极大延长使用寿命
 */
#include "main.h"
/* ===================== 全局参数结构体 ===================== */
// 系统运行时使用的全局参数，存储当前亮度、脉冲宽度、工作模式
uint32_t FirstPage = 0, NbOfPages = 0;
static FLASH_EraseInitTypeDef EraseInitStruct = {0};
ParamStore_t g_params = {
    .brightness  = {127, 127, 127, 127},   // 4路通道默认亮度(500)
    .pulse_width = {0, 0, 0, 0},           // 4路通道默认脉冲宽度(0ms)
    .work_mode   = MODE_ALWAYS_ON,         // 默认工作模式：常亮模式
    .padding     = {0, 0, 0},              // 内存对齐填充字节(无实际功能)
    .magic       = 0x4C454421,             // 校验码"LED!"，用于判断参数是否有效
};

// Flash保存请求标志：true=需要保存参数，false=无需保存
static volatile bool     save_requested = false;
// 延时保存计时：记录最后一次参数修改的时间戳(ms)
static volatile uint32_t save_tick      = 0;

/* ===================== 出厂默认参数 ===================== */
/**
 * @brief 出厂默认参数
 * @note 当Flash中无有效参数（首次上电）或参数损坏时，加载此套默认值
 *       不会被修改，仅作为恢复出厂设置的基准参数
 */
static const ParamStore_t default_params = {
    .brightness  = {30, 30, 30, 30},   // 出厂默认：4路通道亮度均为222
    .pulse_width = {127,127, 127, 127},           // 出厂默认：通道1脉冲宽度1ms，其余0ms
    .work_mode   = MODE_ALWAYS_ON,         // 出厂默认工作模式：常亮模式
    .padding     = {0, 0, 0},              // 对齐填充(固定值，无功能)
    .magic       = 0x4C454421,             // 默认参数校验码，与全局参数保持一致
};

/**
 * @brief 从Flash加载参数到全局变量g_params
 * @retval 无
 * @note 上电时调用，校验Flash参数有效性，无效则加载默认参数
 */
void Params_Load(void)
{
    // 将Flash存储地址强制转换为参数结构体指针
    const ParamStore_t *flash = (const ParamStore_t *)FLASH_STORAGE_ADDR;

    // 校验Flash中的参数校验码，判断参数是否有效
    if (flash->magic == 0x4C454421) {
        // Flash参数有效 → 直接复制到全局运行参数
        memcpy(&g_params, flash, sizeof(ParamStore_t));
    } else {
        // 首次上电/参数损坏 → 加载出厂默认参数
        memcpy(&g_params, &default_params, sizeof(ParamStore_t));
				Params_Save();
    }
}

/**
 * @brief 将全局参数写入Flash（先擦除页，再写入）
 * @retval 无
 * @note STM32G030 Flash编程规则：必须按64位(双字/8字节)写入
 */
void Params_Save(void)
{
    HAL_StatusTypeDef status;

    // 解锁Flash：写入/擦除前必须解锁
	    // 2. 关闭全局中断（避免Flash操作被打断）
    __disable_irq();
    HAL_FLASH_Unlock();

    // Flash擦除配置：仅擦除参数存储的最后1页(页31)
  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.Page        = GetPage(FLASH_STORAGE_ADDR);
  EraseInitStruct.NbPages     = 1;
		EraseInitStruct.Banks=  FLASH_BANK_1;
    uint32_t page_error = 0;  // 擦除错误码
    // 执行Flash页擦除
	 __HAL_FLASH_CLEAR_FLAG(FLASH_SR_CLEAR); 
    status = HAL_FLASHEx_Erase(&EraseInitStruct, &page_error);
    if (status != HAL_OK) {
			NbOfPages=FLASH->SR;
        HAL_FLASH_Lock();  // 擦除失败，上锁并退出
        return;
    }
    // Flash编程：按64位双字写入数据
    const uint64_t *src  = (const uint64_t *)&g_params;  // 源数据：全局参数
    uint32_t        addr = FLASH_STORAGE_ADDR;           // 目标Flash地址
    // 计算需要写入的双字数量（向上取整）
    uint32_t        words = (sizeof(ParamStore_t) + 7) / 8;

    // 循环写入所有参数
    for (uint32_t i = 0; i < words; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                   addr + i * 8, src[i]);
        if (status != HAL_OK)  // 写入失败则退出
            break;
    }

    // 写入完成，锁定Flash
    HAL_FLASH_Lock();
		    // 8. 开启全局中断
    __enable_irq();
}

/**
 * @brief 参数修改后请求保存（按键/串口调节参数时调用）
 * @retval 无
 * @note 重置30秒延时计时器，不会立即写入Flash
 */
void Params_RequestSave(void)
{
    save_requested = true;    // 标记需要保存参数
    save_tick = g_tick_ms;    // 更新最后一次修改的时间戳
}

/**
 * @brief 参数保存定时检查（主循环中周期性调用）
 * @retval 无
 * @note 检测是否达到30秒延时，满足条件则执行Flash写入
 */
void Params_SaveTick(void)
{
    // 无保存请求，直接退出
    if (!save_requested)
        return;

    // 距离最后一次修改满30秒 → 执行保存
    if ((g_tick_ms - save_tick) >= SAVE_DELAY_MS) {
        save_requested = false;  // 清除保存请求
        Params_Save();           // 写入Flash
    }
}

 /**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
  return (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;;
}

