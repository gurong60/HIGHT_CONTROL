/**
 * @file ch455g.c - CH455G 7-segment display + LED driver
 * @brief Bit-bang I2C on PB10(SCL), PB11(SDA)
 * @note  严格适配官方CH455 I2C协议，共阳极数码管(低电平点亮)
 *
 *  CH455G 命令格式：16位(2字节)
 *  高字节：(命令>>7) & 0x3E | 0x40 (I2C地址)
 *  低字节：命令/数据
 *
 *  数码管寄存器：
 *  DIG0(0x1400)、DIG1(0x1500)、DIG2(0x1600)、DIG3(0x1700)
 *
 *  LED/按键寄存器：0x0700(读按键)、0x48(LED控制)
 *
 *  7段映射(共阳极, bit0~bit6: a~g, bit7:小数点)
 *  bit: 7 6 5 4 3 2 1 0
 *       dp a b c d e f g
 */
#include "main.h"


extern ParamStore_t g_params;

/* ------------------- 显示模式枚举 ------------------- */
//typedef enum {
//    DISP_CHANNEL_1 = 0,
//    DISP_CHANNEL_2,
//    DISP_CHANNEL_3,
//    DISP_CHANNEL_4,
//    DISP_MODE_SELECT,
//    DISP_PULSE_WIDTH
//} DisplayMode_t;

/* ------------------- GPIO方向切换(I2C开漏必需) ------------------- */
static void SDA_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CH455_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(CH455_GPIO, &GPIO_InitStruct);
}

static void SDA_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = CH455_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CH455_GPIO, &GPIO_InitStruct);
}

/* ------------------- I2C位操作 ------------------- */
static void SCL_H(void) { HAL_GPIO_WritePin(CH455_GPIO, CH455_SCL_PIN, GPIO_PIN_SET); }
static void SCL_L(void) { HAL_GPIO_WritePin(CH455_GPIO, CH455_SCL_PIN, GPIO_PIN_RESET); }
static void SDA_H(void) { HAL_GPIO_WritePin(CH455_GPIO, CH455_SDA_PIN, GPIO_PIN_SET); }
static void SDA_L(void) { HAL_GPIO_WritePin(CH455_GPIO, CH455_SDA_PIN, GPIO_PIN_RESET); }
static uint8_t SDA_READ(void) { return HAL_GPIO_ReadPin(CH455_GPIO, CH455_SDA_PIN); }

/* 微秒延时(匹配官方4个NOP时序) */
static void delay_us(uint32_t us)
{
    uint32_t ticks = us * (SystemCoreClock / 1000000U) / 4;
    while (ticks--) __NOP();
}

/* ------------------- 官方标准I2C时序 ------------------- */
// I2C起始信号(严格匹配官方驱动)
static void i2c_start(void)
{
    SDA_Output();
    SDA_H();
    SCL_H();
    delay_us(2);
    SDA_L();
    delay_us(2);
    SCL_L();
}

// I2C停止信号(严格匹配官方驱动)
static void i2c_stop(void)
{
    SDA_Output();
    SDA_L();
    delay_us(2);
    SCL_H();
    delay_us(2);
    SDA_H();
    delay_us(2);
    SDA_Input();
}

// 写字节(匹配官方时序)
static void i2c_write_byte(uint8_t data)
{
    SDA_Output();
    for (uint8_t i = 0; i < 8; i++)
    {
        if (data & 0x80) SDA_H();
        else SDA_L();
        delay_us(1);
        SCL_H();
        data <<= 1;
        delay_us(1);
        SCL_L();
    }
    // 读取ACK(忽略)
    SDA_Input();
    SDA_H();
    delay_us(1);
    SCL_H();
    delay_us(2);
    SCL_L();
}

// 读字节(官方按键读取用)
static uint8_t i2c_read_byte(void)
{
    uint8_t dat = 0;
    SDA_Input();
    for (uint8_t i = 0; i < 8; i++)
    {
        dat <<= 1;
        SCL_H();
        delay_us(2);
        if (SDA_READ()) dat++;
        SCL_L();
        delay_us(1);
    }
    // 发送NACK
    SDA_Output();
    SDA_H();
    delay_us(1);
    SCL_H();
    delay_us(2);
    SCL_L();
    return dat;
}

/* ------------------- CH455G 标准命令发送 ------------------- */
 void ch455g_write(uint16_t cmd)
{
    uint8_t high_byte = ((uint8_t)(cmd >> 7) & CH455_I2C_MASK) | CH455_I2C_ADDR;
    uint8_t low_byte  = (uint8_t)cmd;

    i2c_start();
    i2c_write_byte(high_byte);
    i2c_write_byte(low_byte);
    i2c_stop();
    delay_us(50);
}

/* ------------------- 公开函数 ------------------- */

/**
 * @brief  CH455G初始化
 * @note   先开启芯片, 清显示, 关LED
 */
void CH455G_Init(void)
{
    // 初始化GPIO
    SDA_Output();
    SCL_H();
    SDA_H();

    // 官方开启命令: 打开显示+键盘
    ch455g_write(CH455_SYSON|CH455_BIT_INTENS2);
    // 清空4位数码管
    ch455g_write(CH455_DIG0 | SEG_OFF);
    ch455g_write(CH455_DIG1 | SEG_OFF);
    ch455g_write(CH455_DIG2 | SEG_OFF);
    ch455g_write(CH455_DIG3 | SEG_OFF);

    // 关闭所有LED(bit0~3对应LED1~4, 低电平点亮)
    ch455g_write(CH455_LED_CTRL | 0x0F);

    // 默认显示测试
}

/**
 * @brief  显示3位数字+通道号
 * @param  ch: 通道0~3 → 显示1~4; 其他→H    
 * @param  s: 模式1灯
 * @param  p: 模式2灯
 * @param  num: 0~999
 */
void CH455G_DisplayNumber(uint8_t ch, uint16_t num,uint8_t s, uint8_t p)
{
    if (num > 999) num = 999;

    // 第0位: 通道指示
    uint8_t d0;
    if (ch < 4) d0 = seg_font[ch + 1];  // 1~4
    else d0 = SEG_H;                     // H

    // 第1~3位: 百/十/个
    uint8_t d1 = seg_font[(num / 100) % 10];
    uint8_t d2 = seg_font[(num / 10) % 10];
    uint8_t d3 = seg_font[num % 10];
		if (s) d1=(d1|0x80);
    if (p) d2=(d2|0x80);
    // 写入官方寄存器
    ch455g_write(CH455_DIG0 | (d0|0x80));
    ch455g_write(CH455_DIG1 | d1);
    ch455g_write(CH455_DIG2 | d2);
    ch455g_write(CH455_DIG3 | d3);
}


/**
 * @brief  读取按键(官方协议)
 * @retval 按键码, 0=无按键
 */
uint8_t CH455G_ReadKey(void)
{
    uint8_t key;
    uint8_t high_byte = ((uint8_t)(CH455_GET_KEY >> 7) & CH455_I2C_MASK) | CH455_I2C_ADDR | 0x01;

    i2c_start();
    i2c_write_byte(high_byte);
    key = i2c_read_byte();
    i2c_stop();

    return key;
}

/**
 * @brief  显示刷新(按模式更新)
 */
void CH455G_DisplayUpdate(DisplayMode_t mode, uint8_t channel, uint16_t value)
{
    switch (mode)
    {
        case DISP_CHANNEL_1:
        case DISP_CHANNEL_2:
        case DISP_CHANNEL_3:
        case DISP_CHANNEL_4:
            CH455G_DisplayNumber(mode, value,(g_params.work_mode == 1), (g_params.work_mode == 2));
            break;

        case DISP_MODE_SELECT:
            // 显示 H + 工作模式
						if(g_params.work_mode==0)
						{
							ch455g_write(CH455_DIG0 | (SEG_H|0x80));
							ch455g_write(CH455_DIG1 | SEG_OFF);
							ch455g_write(CH455_DIG2 | SEG_OFF);
							ch455g_write(CH455_DIG3 | seg_font[g_params.work_mode]);
						}
						else if(g_params.work_mode==1)
						{
							ch455g_write(CH455_DIG0 | (SEG_H|0x80));
							ch455g_write(CH455_DIG1 | (SEG_OFF|0x80));
							ch455g_write(CH455_DIG2 | (SEG_OFF));
							ch455g_write(CH455_DIG3 | seg_font[g_params.work_mode]);
						}
						else
						{
							ch455g_write(CH455_DIG0 | (SEG_H|0x80));
							ch455g_write(CH455_DIG1 | (SEG_OFF));
							ch455g_write(CH455_DIG2 | (SEG_OFF|0x80));
							ch455g_write(CH455_DIG3 | seg_font[g_params.work_mode]);
						}	
            break;

        case DISP_PULSE_WIDTH:
            CH455G_DisplayNumber(channel, g_params.pulse_width[channel],1,1);
            break;

        default:
            break;
    }
}
