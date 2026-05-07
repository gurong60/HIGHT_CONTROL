/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define OC_THRESHOLD   1500U   /* 过流计算 0.05*3a*10倍*1000=1500mV */
#define OC_CONSECUTIVE  1000U      /*出现过流多少次判断为过流确认 */
/* ======================================================================
 *  PWM Frequency Selection - Use exactly ONE macro at a time
 *  PWM_FREQ_64K  =  64kHz (period=750, resolution 0-749)
 *  PWM_FREQ_250K = 250kHz (period=192, resolution 0-191)
 *  PWM_FREQ_500K = 500kHz (period=96,  resolution 0-95)
 * ====================================================================== */
#define PWM_FREQ_64K
// #define PWM_FREQ_250K
// #define PWM_FREQ_500K
/* ---------- derived constants (do not edit) ---------- */
#if defined(PWM_FREQ_64K)
  /* 64MHz / (0+1) / (999+1) = 64kHz, resolution 0-999 (matches manual) */
  #define PWM_PRESCALER   0U
  #define PWM_ARR         999U
  #define PWM_RESOLUTION  1000U
#elif defined(PWM_FREQ_250K)
  /* 64MHz / (0+1) / (255+1) = 250kHz, resolution 0-255 */
  #define PWM_PRESCALER   0U
  #define PWM_ARR         255U
  #define PWM_RESOLUTION  256U
#elif defined(PWM_FREQ_500K)
  /* 64MHz / (0+1) / (127+1) = 500kHz, resolution 0-127 */
  #define PWM_PRESCALER   0U
  #define PWM_ARR         127U
  #define PWM_RESOLUTION  128U
#else
  #error "Please select exactly one PWM frequency: PWM_FREQ_64K, PWM_FREQ_250K, or PWM_FREQ_500K"
#endif

 /* ---------- brightness scaling: map 0-999 �� 0-PWM_ARR ---------- */
#define BRIGHTNESS_MAX  999U

/* ---------- GPIO Pin Definitions (per schematic) ---------- */
/* Trigger inputs PA4-PA7 (directly on MCU pins, optocoupler active-high) */
#define TRIG1_PIN       GPIO_PIN_4
#define TRIG2_PIN       GPIO_PIN_5
#define TRIG3_PIN       GPIO_PIN_6
#define TRIG4_PIN       GPIO_PIN_7
#define TRIG_GPIO       GPIOA

/* Button inputs PB10(CH/Left), PB11(Middle), PB11(Right) - active low, pull-up */
#define KEY_L_PIN       GPIO_PIN_10
#define KEY_M_PIN       GPIO_PIN_11
#define KEY_R_PIN       GPIO_PIN_12
#define KEY_GPIO        GPIOB

/* USART1: PA9=TX, PA10=RX (RS232 via SP3232EEN) */
/* I2C1:  PB8=SCL, PB9=SDA (CH455G display) �� conflict! PB8 used by KEY_R
 * Schematic shows SCL/SDA on connector CN2, I2C likely uses PA9/PA10 alternate
 * but those are USART too. Actual: CH455G uses bit-bang on PB10(SCL)/PB11(SDA) */
#define CH455_SCL_PIN   GPIO_PIN_13
#define CH455_SDA_PIN   GPIO_PIN_14
#define CH455_GPIO      GPIOB

/* Power enable (optional, from SY8201 EN = PA3 per schematic U1 EN=pin3, but
 * EN is pulled high via R21 100k��, so always enabled unless MCU pulls low) */
/* Not directly controlled by MCU �� skip */

/* LED indicators: active on CH455G extra outputs via display chip, or direct GPIO */
/* From schematic: LED1-LED4 are driven by CH455G, not MCU GPIO */
//#define LED_B_PIN       GPIO_PIN_12  /* Power LED �� PB12 if directly driven */
//#define LED_S_PIN       GPIO_PIN_13  /* Mode1 LED */
//#define LED_P_PIN       GPIO_PIN_14  /* Mode2 LED */
//#define LED_GPIO        GPIOB

/* ADC current sense: PA0 */
#define ADC_CURRENT_CHANNEL  ADC_CHANNEL_0

/* ---------- Flash storage address (last 2KB of 64KB Flash) ---------- */
#define FLASH_STORAGE_ADDR   0x0800f800U
//#define FLASH_PAGE_SIZE      2048U  /* STM32G030: 2KB per page */

/* ---------- RS232 Protocol ---------- */
#define RS232_BAUDRATE       115200U
#define RS232_RX_BUF_SIZE    64

/* ---------- Timing ---------- */
#define SAVE_DELAY_MS        5000U   /* 30 seconds delayed save */
#define DEBOUNCE_MS          20U
#define LONG_PRESS_MS        800U
#define RAPID_STEP_MS        20U




/* ------------------- 硬件引脚定义 ------------------- */
#define CH455_GPIO        GPIOB
#define CH455_SCL_PIN     GPIO_PIN_13  // PB13 -> SCL
#define CH455_SDA_PIN     GPIO_PIN_14  // PB14 -> SDA

/* ------------------- CH455 官方命令定义 ------------------- */
#define CH455_I2C_ADDR    0x40        // 官方I2C写地址
#define CH455_I2C_MASK    0x3E        // 命令掩码
#define CH455_SYSON       0x0401      // 开启显示+键盘(官方)
#define CH455_SYSOFF      0x0400      // 关闭显示+键盘
#define CH455_DIG0        0x1400      // 数码管0
#define CH455_DIG1        0x1500      // 数码管1
#define CH455_DIG2        0x1600      // 数码管2
#define CH455_DIG3        0x1700      // 数码管3
#define CH455_GET_KEY     0x0700      // 读取按键
#define CH455_LED_CTRL    0x4800      // LED控制命令



// 设置系统参数命令

#define CH455_BIT_ENABLE	0x01		// 开启/关闭位
#define CH455_BIT_SLEEP		0x04		// 睡眠控制位
#define CH455_BIT_7SEG		0x08		// 7段控制位
#define CH455_BIT_INTENS1	0x10		// 1级亮度
#define CH455_BIT_INTENS2	0x20		// 2级亮度
#define CH455_BIT_INTENS3	0x30		// 3级亮度
#define CH455_BIT_INTENS4	0x40		// 4级亮度
#define CH455_BIT_INTENS5	0x50		// 5级亮度
#define CH455_BIT_INTENS6	0x60		// 6级亮度
#define CH455_BIT_INTENS7	0x70		// 7级亮度
#define CH455_BIT_INTENS8	0x00		// 8级亮度

/* ------------------- 段码定义(共阳极, 0=亮, 1=灭) ------------------- */
// 标准CH455 7段码: a b c d e f g dp
static const uint8_t seg_font[10] = {
    0x3F, // 0: a b c d e f
    0x06, // 1: b c
    0x5B, // 2: a b g e d
    0x4F, // 3: a b g c d
    0x66, // 4: f g b c
    0x6D, // 5: a f g c d
    0x7D, // 6: a f g c d e
    0x07, // 7: a b c
    0x7F, // 8: 全亮
    0x6F  // 9: a b c d f g
};

#define SEG_H       0x76  // 字符H
#define SEG_DASH    0x40  // 横杠-
#define SEG_DP      0x80  // 小数点
#define SEG_OFF     0x00  // 全灭

/* ---------- Work Modes ---------- */
typedef enum {
    MODE_ALWAYS_ON  = 0,   /* Constant brightness */
    MODE_TRIGGER_1  = 1,   /* Level-triggered: high=on */
    MODE_TRIGGER_2  = 2,   /* Edge-triggered: pulse output */
} WorkMode_t;
// 编译期校验：结构体大小必须是8的整数倍

/* ---------- Parameter Storage Structure ---------- */
typedef struct {
    uint16_t brightness[4];   /* 0-999 per channel */
    uint16_t pulse_width[4];  /* 0-999 ms, mode2 only */
    uint8_t  work_mode;       /* 0/1/2 */
    uint8_t  padding[3];      /* alignment */
    uint32_t magic;           /* 0x4C454421 "LED!" */
} ParamStore_t;

/* ---------- Display ---------- */
typedef enum {
    DISP_CHANNEL_1 = 0,
    DISP_CHANNEL_2,
    DISP_CHANNEL_3,
    DISP_CHANNEL_4,
    DISP_MODE_SELECT,
    DISP_PULSE_WIDTH,
} DisplayMode_t;

/* ---------- Global Variables (defined in main.c) ---------- */
extern volatile uint32_t g_tick_ms;
  extern ParamStore_t g_params;
 extern  DisplayMode_t disp_mode;  // 当前显示模式，默认通道1
extern uint8_t  cur_channel; 
/* ---------- Function Prototypes ---------- */
/* main.c */
void SystemClock_Config(void);
void Error_Handler(void);

/* gpio.c */
void MX_GPIO_Init(void);

/* tim.c */
void MX_TIM1_Init(void);
void Set_PWM_Duty(uint8_t ch, uint16_t brightness);

/* usart.c */
void MX_USART1_UART_Init(void);
void RS232_Process(uint8_t size);
void UART_Send(const char *str);

/* ch455g.c */
void CH455G_Init(void);
 void ch455g_write(uint16_t cmd);
void CH455G_DisplayNumber(uint8_t ch, uint16_t num,uint8_t s, uint8_t p);
void CH455G_DisplayUpdate(DisplayMode_t mode, uint8_t channel,
                           uint16_t value);

/* buttons.c */
void Buttons_Init(void);
void Buttons_Process(void);

/* params.c */
void Params_Load(void);
void Params_Save(void);
void Params_RequestSave(void);
void Params_SaveTick(void);

/* trigger.c */
void Trigger_Init(void);
void Trigger_Process(void);
void Trigger_EXTI_Callback (uint8_t ch);
void Trigger_Rising_Callback (uint8_t ch);
/* overcurrent.c */
void Overcurrent_Init(void);
void Overcurrent_Process(void);
uint32_t GetPage(uint32_t Addr);
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
