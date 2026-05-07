;/**
; *******************************************************************************
; * @file    startup_stm32g030xx.s
; * @brief   STM32G030xx 设备启动文件 (ARM Compiler 6 / Keil MDK)
; *          - 初始化堆栈
; *          - 定义中断向量表
; *          - 复位处理入口
; *******************************************************************************
; */

; 栈大小配置
Stack_Size      EQU     0x00000400

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

; 堆大小配置
Heap_Size       EQU     0x00000200

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB

; ========== 中断向量表 ==========
                AREA    RESET, DATA, READONLY
                EXPORT  __Vectors
                EXPORT  __Vectors_End
                EXPORT  __Vectors_Size

__Vectors       DCD     __initial_sp               ; 0: 初始栈指针
                DCD     Reset_Handler              ; 1: 复位处理
                DCD     NMI_Handler                ; 2: NMI
                DCD     HardFault_Handler          ; 3: Hard Fault
                DCD     0                          ; 4: Reserved
                DCD     0                          ; 5: Reserved
                DCD     0                          ; 6: Reserved
                DCD     0                          ; 7: Reserved
                DCD     0                          ; 8: Reserved
                DCD     0                          ; 9: Reserved
                DCD     0                          ; 10: Reserved
                DCD     SVC_Handler                ; 11: SVCall
                DCD     0                          ; 12: Reserved
                DCD     0                          ; 13: Reserved
                DCD     PendSV_Handler             ; 14: PendSV
                DCD     SysTick_Handler            ; 15: SysTick

                ; STM32G030 外部中断
                DCD     WWDG_IRQHandler                ; 0
                DCD     PVD_VDDIO2_IRQHandler          ; 1
                DCD     RTC_TAMP_IRQHandler             ; 2
                DCD     FLASH_IRQHandler                ; 3
                DCD     RCC_CRS_IRQHandler              ; 4
                DCD     EXTI0_1_IRQHandler              ; 5
                DCD     EXTI2_3_IRQHandler              ; 6
                DCD     EXTI4_15_IRQHandler             ; 7
                DCD     0                               ; 8
                DCD     DMA1_Channel1_IRQHandler        ; 9
                DCD     DMA1_Channel2_3_IRQHandler      ; 10
                DCD     DMA1_Ch4_7_DMA2_Ch1_2_IRQHandler; 11
                DCD     ADC1_COMP_IRQHandler            ; 12
                DCD     TIM1_BRK_UP_TRG_COM_IRQHandler  ; 13
                DCD     TIM1_CC_IRQHandler              ; 14
                DCD     TIM2_IRQHandler                 ; 15
                DCD     TIM3_TIM4_IRQHandler            ; 16
                DCD     TIM6_DAC_LPTIM1_IRQHandler      ; 17
                DCD     TIM7_LPTIM2_IRQHandler          ; 18
                DCD     TIM14_IRQHandler                ; 19
                DCD     TIM15_IRQHandler                ; 20
                DCD     TIM16_FDCAN_IT0_IRQHandler      ; 21
                DCD     TIM17_FDCAN_IT1_IRQHandler      ; 22
                DCD     I2C1_IRQHandler                 ; 23
                DCD     I2C2_IRQHandler                 ; 24
                DCD     SPI1_IRQHandler                 ; 25
                DCD     SPI2_IRQHandler                 ; 26
                DCD     USART1_IRQHandler               ; 27
                DCD     USART2_IRQHandler               ; 28
                DCD     USART3_4_5_6_LPUART1_IRQHandler ; 29
                DCD     0                               ; 30
                DCD     USB_IRQHandler                  ; 31

__Vectors_End

__Vectors_Size  EQU     __Vectors_End - __Vectors

; ========== 复位处理 ==========
                AREA    |.text|, CODE, READONLY

Reset_Handler   PROC
                EXPORT  Reset_Handler                [WEAK]
                IMPORT  SystemInit
                IMPORT  __main

                LDR     R0, =SystemInit
                BLX     R0
                LDR     R0, =__main
                BX      R0
                ENDP

; ========== 默认中断处理 ==========
                ALIGN

NMI_Handler             PROC
                        EXPORT  NMI_Handler                [WEAK]
                        B       .
                        ENDP

HardFault_Handler       PROC
                        EXPORT  HardFault_Handler          [WEAK]
                        B       .
                        ENDP

SVC_Handler             PROC
                        EXPORT  SVC_Handler                [WEAK]
                        B       .
                        ENDP

PendSV_Handler          PROC
                        EXPORT  PendSV_Handler             [WEAK]
                        B       .
                        ENDP

SysTick_Handler         PROC
                        EXPORT  SysTick_Handler            [WEAK]
                        B       .
                        ENDP

Default_Handler         PROC
                        EXPORT  WWDG_IRQHandler                [WEAK]
                        EXPORT  PVD_VDDIO2_IRQHandler          [WEAK]
                        EXPORT  RTC_TAMP_IRQHandler             [WEAK]
                        EXPORT  FLASH_IRQHandler                [WEAK]
                        EXPORT  RCC_CRS_IRQHandler              [WEAK]
                        EXPORT  EXTI0_1_IRQHandler              [WEAK]
                        EXPORT  EXTI2_3_IRQHandler              [WEAK]
                        EXPORT  EXTI4_15_IRQHandler             [WEAK]
                        EXPORT  DMA1_Channel1_IRQHandler        [WEAK]
                        EXPORT  DMA1_Channel2_3_IRQHandler      [WEAK]
                        EXPORT  DMA1_Ch4_7_DMA2_Ch1_2_IRQHandler [WEAK]
                        EXPORT  ADC1_COMP_IRQHandler            [WEAK]
                        EXPORT  TIM1_BRK_UP_TRG_COM_IRQHandler  [WEAK]
                        EXPORT  TIM1_CC_IRQHandler              [WEAK]
                        EXPORT  TIM2_IRQHandler                 [WEAK]
                        EXPORT  TIM3_TIM4_IRQHandler            [WEAK]
                        EXPORT  TIM6_DAC_LPTIM1_IRQHandler      [WEAK]
                        EXPORT  TIM7_LPTIM2_IRQHandler          [WEAK]
                        EXPORT  TIM14_IRQHandler                [WEAK]
                        EXPORT  TIM15_IRQHandler                [WEAK]
                        EXPORT  TIM16_FDCAN_IT0_IRQHandler      [WEAK]
                        EXPORT  TIM17_FDCAN_IT1_IRQHandler      [WEAK]
                        EXPORT  I2C1_IRQHandler                 [WEAK]
                        EXPORT  I2C2_IRQHandler                 [WEAK]
                        EXPORT  SPI1_IRQHandler                 [WEAK]
                        EXPORT  SPI2_IRQHandler                 [WEAK]
                        EXPORT  USART1_IRQHandler               [WEAK]
                        EXPORT  USART2_IRQHandler               [WEAK]
                        EXPORT  USART3_4_5_6_LPUART1_IRQHandler [WEAK]
                        EXPORT  USB_IRQHandler                  [WEAK]

WWDG_IRQHandler
PVD_VDDIO2_IRQHandler
RTC_TAMP_IRQHandler
FLASH_IRQHandler
RCC_CRS_IRQHandler
EXTI0_1_IRQHandler
EXTI2_3_IRQHandler
EXTI4_15_IRQHandler
DMA1_Channel1_IRQHandler
DMA1_Channel2_3_IRQHandler
DMA1_Ch4_7_DMA2_Ch1_2_IRQHandler
ADC1_COMP_IRQHandler
TIM1_BRK_UP_TRG_COM_IRQHandler
TIM1_CC_IRQHandler
TIM2_IRQHandler
TIM3_TIM4_IRQHandler
TIM6_DAC_LPTIM1_IRQHandler
TIM7_LPTIM2_IRQHandler
TIM14_IRQHandler
TIM15_IRQHandler
TIM16_FDCAN_IT0_IRQHandler
TIM17_FDCAN_IT1_IRQHandler
I2C1_IRQHandler
      EXPORT  S
SPI1_IRQHandler
SPI2_IRQHandler
USART1_IRQHandler
USART2_IRQHandler
USART3_4_5_6_LPUART1_IRQHandler
USB_IRQHandler
                        B       .
                        ENDP

                ALIGN

; ========== 堆栈初始化 ==========
                IF      :DEF:__MICROLIB

                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit

                ELSE

                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap

__user_initial_stackheap
                LDR     R0, =Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, =(Heap_Mem + Heap_Size)
                LDR     R3, =Stack_Mem
                BX      LR

                ALIGN

                ENDIF

                END
