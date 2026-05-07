/**
 * @file buttons.c - 带消抖和长按检测的按键扫描驱动
 * @brief 按键定义：
 *        KEY_L(PB6) = 通道选择键(CH)
 *        KEY_M(PB7) = 减号键(-)
 *        KEY_R(PB8) = 加号键(+)
 *
 * 按键功能定义：
 *  1. CH键短按：循环切换 通道1→2→3→4→模式选择→通道1
 *  2. 工作模式2下，CH键长按：切换 脉冲宽度调节模式
 *  3. +/-键短按：单次调节数值(亮度/脉冲宽度/工作模式)
 *  4. +/-键长按：快速连续调节数值
 *
 *  注意：PB8与硬件I2C1_SCL引脚冲突
 *        本工程使用软件模拟I2C(PB10/PB11)驱动CH455G数码管
 *        因此PB8可正常用于KEY_R，已通过原理图验证
 */
#include "main.h"
#include "stm32g0xx_hal.h"
/**
 * @brief 按键状态机枚举
 * @details 定义按键的4种工作状态，用于状态机扫描
 */
typedef enum {
    BTN_IDLE = 0,      // 空闲状态：按键未按下，初始状态
    BTN_DEBOUNCE,      // 消抖状态：按键刚按下，等待电平稳定
    BTN_PRESSED,       // 确认按下状态：消抖完成，按键有效按下
    BTN_LONG,          // 长按状态：按下时间超过长按阈值
} BtnState_t;

/**
 * @brief 按键控制结构体
 * @details 存储单个按键的硬件信息、状态、计时变量
 */
typedef struct {
    GPIO_TypeDef *port;      // 按键对应的GPIO端口 (如GPIOB)
    uint16_t      pin;       // 按键对应的GPIO引脚 (如GPIO_PIN_6)
    BtnState_t    state;     // 按键当前状态机状态
    uint32_t      press_tick;// 按键按下的起始时间戳(ms)，用于消抖/长按计时
    uint32_t      repeat_tick;// 长按连发的时间戳(ms)，用于快速调节
} Button_t;

/* 定义3个按键实例：左(CH)、中(-)、右(+)，初始化默认空闲状态 */
static Button_t btn_l = { KEY_GPIO, KEY_L_PIN, BTN_IDLE, 0, 0 };
static Button_t btn_m = { KEY_GPIO, KEY_M_PIN, BTN_IDLE, 0, 0 };
static Button_t btn_r = { KEY_GPIO, KEY_R_PIN, BTN_IDLE, 0, 0 };

/* ------------------- 全局显示/模式状态变量 ------------------- */
 DisplayMode_t disp_mode = DISP_CHANNEL_1;  // 当前显示模式，默认通道1
uint8_t  cur_channel   = 0;                // 当前选中的通道号(0~4)
static bool     in_pulse_mode = false;            // 是否进入脉冲宽度调节模式

/**
 * @brief  读取按键电平
 * @param  b: 按键结构体指针
 * @retval true=按键按下, false=按键松开
 * @note   硬件为低电平有效：GPIO复位=0 → 按下
 */
static inline bool btn_read(Button_t *b)
{
    return (HAL_GPIO_ReadPin(b->port, b->pin) == GPIO_PIN_RESET);
}

/**
 * @brief  数值调节公共函数(+/-按键共用)
 * @param  b: 按键结构体指针
 * @param  is_positive: true=加号调节, false=减号调节
 * @details 根据当前工作模式，执行不同的调节逻辑：
 *          1. 模式选择界面：调节工作模式(0/1/2)
 *          2. 脉冲宽度模式：调节对应通道的脉冲宽度
 *          3. 普通亮度模式：调节对应通道的亮度(PWM占空比)
 *          调节完成后刷新显示+保存参数
 */
static void do_adjust(Button_t *b, bool is_positive)
{
    // 场景1：当前处于【工作模式选择】界面
    if (disp_mode == DISP_MODE_SELECT) {
        if (is_positive) {
            // 加号：工作模式+1，上限2，循环到0
            g_params.work_mode++;
            if (g_params.work_mode > 2) g_params.work_mode = 0;
        } else {
            // 减号：工作模式-1，下限0，循环到2
            if (g_params.work_mode == 0) g_params.work_mode = 2;
            else g_params.work_mode--;
        }
				if(g_params.work_mode==0)
				{
					 	Set_PWM_En(0,true);
						Set_PWM_En(1,true);
						Set_PWM_En(2,true);
						Set_PWM_En(3,true);
				}
				else if(g_params.work_mode==1)
				{
						if(HAL_GPIO_ReadPin(TRIG_GPIO,TRIG1_PIN)==0)
						{
							 Set_PWM_En(0,true);
						}
						else
						{
							  Set_PWM_En(0,false);
						}
						if(HAL_GPIO_ReadPin(TRIG_GPIO,TRIG2_PIN)==0)
						{
							 Set_PWM_En(1,true);
						}
						else
						{
							  Set_PWM_En(1,false);
						}
						if(HAL_GPIO_ReadPin(TRIG_GPIO,TRIG3_PIN)==0)
						{
							Set_PWM_En(2,true);
						}
						else
						{
							  Set_PWM_En(2,false);
						}
						if(HAL_GPIO_ReadPin(TRIG_GPIO,TRIG4_PIN)==0)
						{
							Set_PWM_En(3,true);
						}
						else
						{
							  Set_PWM_En(3,false);
						}
				}
				else
				{
					  Set_PWM_En(0,false);
						Set_PWM_En(1,false);
						Set_PWM_En(2,false);
						Set_PWM_En(3,false);
				}
        // 刷新显示 + 请求保存参数
        CH455G_DisplayUpdate(DISP_MODE_SELECT, 0, 0);
        Params_RequestSave();
    }
    // 场景2：当前处于【脉冲宽度调节】模式
    else if (disp_mode == DISP_PULSE_WIDTH ) {
        if (is_positive) {
            // 加号：脉冲宽度+1，上限999
            if (g_params.pulse_width[cur_channel] < 999)
                g_params.pulse_width[cur_channel]++;
        } else {
            // 减号：脉冲宽度-1，下限0
            if (g_params.pulse_width[cur_channel] > 0)
                g_params.pulse_width[cur_channel]--;
        }
        // 刷新显示 + 请求保存参数
        CH455G_DisplayUpdate(DISP_PULSE_WIDTH, cur_channel,
                             g_params.pulse_width[cur_channel]);
        Params_RequestSave();
    }
    // 场景3：普通亮度调节模式(通道0~3)
    else if (cur_channel < 4) {
        if (is_positive) {
            // 加号：亮度+1，上限999
            if (g_params.brightness[cur_channel] < PWM_ARR)
                g_params.brightness[cur_channel]++;
        } else {
            // 减号：亮度-1，下限0
            if (g_params.brightness[cur_channel] > 0)
                g_params.brightness[cur_channel]--;
        }
        // 更新PWM占空比 + 刷新显示 + 保存参数
        Set_PWM_Duty(cur_channel, g_params.brightness[cur_channel]);
        CH455G_DisplayUpdate(disp_mode, cur_channel,
                             g_params.brightness[cur_channel]);
        Params_RequestSave();
    }
}

/**
 * @brief  单个按键状态机更新函数(核心驱动)
 * @param  b: 按键结构体指针
 * @details 有限状态机：处理消抖、短按、长按、连发逻辑
 */
static void btn_update(Button_t *b)
{
	static bool btn_l_long_trig = false;
    // 读取当前按键电平状态
    bool pressed = btn_read(b);

    // 状态机分支：根据按键当前状态执行不同逻辑
    switch (b->state) {
        // ------------------- 状态1：空闲 -------------------
        case BTN_IDLE:
            // 检测到按键按下 → 进入消抖状态，记录按下时间
            if (pressed) {
                b->state = BTN_DEBOUNCE;
                b->press_tick = g_tick_ms;
            }
            break;

        // ------------------- 状态2：消抖 -------------------
        case BTN_DEBOUNCE:
            // 等待消抖时间完成(DEBOUNCE_MS)
            if (g_tick_ms - b->press_tick >= DEBOUNCE_MS) {
                if (pressed) {
                    // 消抖后仍按下 → 进入有效按下状态
                    b->state = BTN_PRESSED;
                    b->repeat_tick = g_tick_ms;
                } else {
                    // 消抖后松开 → 判定为干扰，回到空闲
                    b->state = BTN_IDLE;
                }
            }
            break;

        // ------------------- 状态3：确认按下(短按等待) -------------------
        case BTN_PRESSED:
            // 按键松开 → 执行【短按】逻辑
            if (!pressed) {
                b->state = BTN_IDLE;
                // CH键(左按键) 短按处理
                if (b == &btn_l) 
								{
                    // 如果当前在脉冲模式 → 退出脉冲模式
                    if (in_pulse_mode) {
                        cur_channel++;
                        if (cur_channel > 3) cur_channel = 0;
                        // 通道=4 → 进入模式选择界面
                   
                    } 
										else
										{
                        // 否则：循环切换通道/模式
                        cur_channel++;
                        if (cur_channel > 4) cur_channel = 0;
                        // 通道=4 → 进入模式选择界面
                        if (cur_channel == 4) {
                            disp_mode = DISP_MODE_SELECT;
                        } else {
                            // 切换到对应通道显示模式
                            disp_mode = (DisplayMode_t)(DISP_CHANNEL_1 + cur_channel);
                        }
                    }
                    // 根据最新模式刷新数码管显示
                    if (disp_mode == DISP_MODE_SELECT)
                        CH455G_DisplayUpdate(DISP_MODE_SELECT, 0, 0);
                    else if (in_pulse_mode)
                        CH455G_DisplayUpdate(DISP_PULSE_WIDTH, cur_channel,
                                             g_params.pulse_width[cur_channel]);
                    else
                        CH455G_DisplayUpdate(disp_mode, cur_channel,
                                             g_params.brightness[cur_channel]);
                }
                // +/-键 短按处理：单次调节数值
                else {
                    do_adjust(b, (b == &btn_r));
                }
                return;
            }
            // 按键未松开，且按下时间超过长按阈值 → 进入长按状态
            if (g_tick_ms - b->press_tick >= LONG_PRESS_MS) {
                b->state = BTN_LONG;
							 // 仅左键：达到长按时间 → 立即执行，不等待松开
                if ((b == &btn_l )&&( disp_mode != DISP_MODE_SELECT)) {
                    btn_l_long_trig = true; // 只触发1次，防止重复
                    // 原长按逻辑：仅模式2下切换脉冲模式
                    
                        in_pulse_mode = !in_pulse_mode;
                        if (in_pulse_mode) {
                            disp_mode = DISP_PULSE_WIDTH;
                            CH455G_DisplayUpdate(DISP_PULSE_WIDTH, cur_channel, g_params.pulse_width[cur_channel]);
                        } else {
                            disp_mode = (DisplayMode_t)(DISP_CHANNEL_1 + cur_channel);
                            CH455G_DisplayUpdate(disp_mode, cur_channel, g_params.brightness[cur_channel]);
                        }
                    
                }
            }
            break;

        // ------------------- 状态4：长按 -------------------
        case BTN_LONG:
            // 长按后松开 → 执行【长按】逻辑
            if (!pressed) {
                b->state = BTN_IDLE;
//                // CH键 长按处理：仅工作模式2下，切换脉冲宽度模式
//                if (b == &btn_l) {

//                        // 切换脉冲模式标志位
//                        in_pulse_mode = !in_pulse_mode;
//                        if (in_pulse_mode) {
//                            // 进入脉冲宽度调节
//                            disp_mode = DISP_PULSE_WIDTH;
//                            CH455G_DisplayUpdate(DISP_PULSE_WIDTH, cur_channel,
//                                                 g_params.pulse_width[cur_channel]);
//                        } else {
//                            // 退出脉冲模式，回到亮度调节
//                            disp_mode = (DisplayMode_t)(DISP_CHANNEL_1 + cur_channel);
//                            CH455G_DisplayUpdate(disp_mode, cur_channel,
//                                                 g_params.brightness[cur_channel]);
//                        }
//                    
//                }
                return;
            }
            // +/-键 长按：快速连发调节(定时执行调节)
            if (b == &btn_r || b == &btn_m) {
                if (g_tick_ms - b->repeat_tick >= RAPID_STEP_MS) {
                    // 达到连发间隔，执行一次调节，并刷新计时
                    b->repeat_tick = g_tick_ms;
                    do_adjust(b, (b == &btn_r));
                }
            }
						
            break;
    }
}

/* ------------------- 公开接口函数 ------------------- */

/**
 * @brief  按键初始化
 * @return 无
 * @details 初始化默认显示模式、通道号、脉冲模式标志
 */
void Buttons_Init(void)
{
    disp_mode = DISP_CHANNEL_1;   // 默认显示通道1
    cur_channel = 0;              // 默认通道0
    in_pulse_mode = false;        // 默认不进入脉冲调节模式
	for (uint8_t ch = 0; ch < 4; ch++)
	 {
        Set_PWM_Duty(ch, g_params.brightness[ch]);
	 }
	if(g_params.work_mode == 0)
	{
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
		HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
		
	}

	

	CH455G_DisplayUpdate(disp_mode, cur_channel,
												g_params.brightness[cur_channel]);
}

/**
 * @brief  按键总处理函数
 * @return 无
 * @details 主循环中周期性调用，扫描3个按键的状态
 */
void Buttons_Process(void)
{
    btn_update(&btn_l);   // 扫描左按键(CH)
    btn_update(&btn_m);   // 扫描中按键(-)
    btn_update(&btn_r);   // 扫描右按键(+)
}

