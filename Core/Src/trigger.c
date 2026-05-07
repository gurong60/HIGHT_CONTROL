/**
 * @file trigger.c - 外部触发输入处理
 * @brief PA4-PA7 对应 TRIG1-TRIG4 触发信号（光耦隔离，高电平有效）
 *
 * 工作模式说明：
 *  模式0（常亮模式）：忽略触发信号 —— LED 保持设定亮度持续点亮
 *  模式1（触发模式1）：高电平 = LED 点亮，低电平 = LED 熄灭
 *  模式2（触发模式2）：上升沿触发 → LED 按设定脉冲宽度点亮指定毫秒数
 *
 * 模式2的脉冲计时基于1ms系统滴答定时器(SysTick)实现，简洁可靠
 */
#include "main.h"
// 引入全局参数结构体（包含亮度、脉冲宽度、工作模式等配置）
extern  uint32_t g_trig;
extern ParamStore_t g_params;
uint8_t mode_0_frist=0;
// 4路通道的脉冲启动时间戳（单位：毫秒）
static uint32_t pulse_start[4] = {0};
// 4路通道的脉冲激活标志（true=脉冲输出中，false=脉冲已结束）
static bool     pulse_active[4] = {false};
// 4路通道的上一次触发电平状态（用于边沿检测）
static bool     trig_prev[4]    = {false};

/* ---- 读取触发输入电平（高电平返回true，表示触发有效） ---- */
static inline bool trig_read(uint8_t ch)
{
    // 定义4路触发信号对应的GPIO引脚数组
    uint16_t pins[] = {TRIG1_PIN, TRIG2_PIN, TRIG3_PIN, TRIG4_PIN};
    // 读取指定通道的触发引脚电平，高电平代表触发信号有效
    return (HAL_GPIO_ReadPin(TRIG_GPIO, pins[ch]) == GPIO_PIN_SET);
}

/**
 * @brief 触发模块初始化
 * @retval 无
 * @note 初始化脉冲状态和触发电平状态
 */
void Trigger_Init(void)
{
    // 遍历4路通道，复位脉冲状态和边沿检测状态
		if(g_params.work_mode == 1)
		{
			for (int i = 0; i < 4; i++) {
					pulse_active[i] = false;
					trig_prev[i]    = true;
			}
		}
		else if(g_params.work_mode == 2)
		{
			for (int i = 0; i < 4; i++) {
					pulse_active[i] = true;
					trig_prev[i] =  false;
			
			}
		}
}

/**
 * @brief 触发信号主处理函数
 * @retval 无
 * @note 需在主循环中周期性调用，处理电平触发/边沿触发逻辑
 */
void Trigger_Process(void)
{

 uint8_t mode = g_params.work_mode;

    // ============== 模式0：常亮模式 ==============
    if(mode == MODE_ALWAYS_ON)
    {
				if(mode_0_frist==0)
				{
					for(uint8_t ch=0; ch<4; ch++)
					{
							Set_PWM_En(ch,true);;
							pulse_active[ch] = false;
					}
					mode_0_frist=1;
					return;
				}
    }

    // ============== 模式2：脉冲超时关闭 ==============
   else if(mode == MODE_TRIGGER_2)
    {
        for(uint8_t ch=0; ch<4; ch++)
        {
						uint32_t pw = g_params.pulse_width[ch]*100;
            if(pulse_active[ch])
            {
                 
                // 脉冲宽度=0：瞬时关闭
                if(pw == 0)
                {
                    Set_PWM_En(ch,false);
                    pulse_active[ch] = false;
                }
                // 计时结束：关闭PWM + 解锁触发
                if(g_trig - (pulse_start[ch]) >= pw)
                {
                   Set_PWM_En(ch,false);
                    pulse_active[ch] = false;
                }
            }
        }
				mode_0_frist=0;
    }
		else if(mode == MODE_TRIGGER_1)
		{
				mode_0_frist=0;
		}


}
void Trigger_EXTI_Callback (uint8_t ch)
{
		uint8_t mode = g_params.work_mode;


    // ============== 模式1：电平触发 ==============
    if(mode == MODE_TRIGGER_1)
    {
        // 实时读取电平，高电平点亮，低电平熄灭
        if(HAL_GPIO_ReadPin(TRIG_GPIO, trig_read(ch)) == GPIO_PIN_RESET)
						Set_PWM_En(ch,true);
        else
            Set_PWM_En(ch,false);
    }

    // ============== 模式2：中断上升沿触发 + 锁存（核心） ==============
    if(mode == MODE_TRIGGER_2)
    {
        // ===================== 关键：脉冲未完成，禁止再次触发 =====================
        if(pulse_active[ch]) return;
				if(g_params.pulse_width[ch]==0)return;
        // 启动脉冲
        Set_PWM_En(ch,true);
        pulse_start[ch] = g_trig;
        pulse_active[ch] = true;  // 锁存：屏蔽后续触发
    }
}

void Trigger_Rising_Callback (uint8_t ch)
{
	   if(g_params.work_mode == MODE_TRIGGER_1)
    {
        // 实时读取电平，高电平点亮，低电平熄灭
        if(HAL_GPIO_ReadPin(TRIG_GPIO, trig_read(ch)) == GPIO_PIN_SET)
						Set_PWM_En(ch,true);
        else
            Set_PWM_En(ch,false);
    }
}
