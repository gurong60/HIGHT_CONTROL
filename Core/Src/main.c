/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
extern uint16_t   aADCxConvertedData[16][2];
 volatile uint32_t g_tick_ms = 0;
extern uint8_t  rx_buf[64];
 extern DisplayMode_t disp_mode;
 uint8_t RxMark=0,RxStart=0;
 uint16_t Cer_ma;
 static uint32_t fault_count = 0,Vault_count;
 uint32_t g_trig=0;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_TIM1_Init();
  MX_TIM17_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */
	Params_Load();
	for (uint8_t i = 0; i < 4; i++) 
	{
		if(g_params.brightness[i]>PWM_ARR)
		{
			 g_params.brightness[i]=PWM_ARR ;
				Params_RequestSave();
		}
	}
	 HAL_TIM_Base_Start_IT(&htim17);

	HAL_Delay(200);
		if(g_params.work_mode!=1)
	{
						 Set_PWM_En(0,false);
				 Set_PWM_En(1,false);
				 Set_PWM_En(2,false);
				 Set_PWM_En(3,false);
	}
	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)aADCxConvertedData, 32);
	CH455G_Init();
	Buttons_Init();
	Trigger_Init();
	
	HAL_UARTEx_ReceiveToIdle_IT(&huart2,&rx_buf[0],64);
//	Read_WRP_OptionBytes();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		if(RxStart==1)
		{
			if(RxMark>=3) 
			{
				RS232_Process(RxMark);
				RxMark=0;
			}
			
			RxStart=0;
		}
		HAL_IWDG_Refresh(&hiwdg);
		Cer_ma=ADC_FIT()*1000;
		if(Cer_ma>OC_THRESHOLD)  //过流检查
		{
			 fault_count++;
			 if (fault_count >= OC_CONSECUTIVE) //持续过流检查
			 {
				 __ASM volatile("CPSID I");   // 汇编指令：置 PRIMASK=1
				 Set_PWM_En(0,false);
				 Set_PWM_En(1,false);
				 Set_PWM_En(2,false);
				 Set_PWM_En(3,false);
				 uint8_t d1= 0X5C;
				 uint8_t d2 = 0X58;
         uint8_t d3 = 0X73;
         uint8_t d0 = 0X79;
		     if (g_params.work_mode==1) d1=(d1|0x80);
         if (g_params.work_mode==2) d2=(d2|0x80);
				// Eocp保护
				ch455g_write(CH455_DIG0 | (d0|0x80));
				ch455g_write(CH455_DIG1 | d1);
				ch455g_write(CH455_DIG2 | d2);
				ch455g_write(CH455_DIG3 | d3);
				 while(1){HAL_IWDG_Refresh(&hiwdg);}
		 	 }
		}
		else
		{
				fault_count=0;
		}
		if((HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_3)&&HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_4)&&HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_5)&&HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_6))==0)
		{
			Vault_count++;
			if(Vault_count>10)
			{
			__ASM volatile("CPSID I");   // 汇编指令：置 PRIMASK=1
				Set_PWM_En(0,false);
				 Set_PWM_En(1,false);
				 Set_PWM_En(2,false);
				 Set_PWM_En(3,false);
				 uint8_t d1= 0X6d;
				 uint8_t d2 = 0X58;
         uint8_t d3 = 0X73;
         uint8_t d0 = 0X79;
		     if (g_params.work_mode==1) d1=(d1|0x80);
         if (g_params.work_mode==2) d2=(d2|0x80);
				// Eocp保护
				ch455g_write(CH455_DIG0 | (d0|0x80));
				ch455g_write(CH455_DIG1 | d1);
				ch455g_write(CH455_DIG2 | d2);
				ch455g_write(CH455_DIG3 | d3);
				 while(1){HAL_IWDG_Refresh(&hiwdg);}
			 }
		}
		else 
		{
			 Vault_count=0;
		}
		Buttons_Process();    /* Scan buttons, adjust brightness/mode */
		Trigger_Process();    /* Handle trigger inputs per work mode */
		Params_SaveTick();    /* Check 30-second delayed save */

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
 * @brief  HAL�жϻص�����
 */
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
    switch(GPIO_Pin)
    {
        case GPIO_PIN_4: Trigger_EXTI_Callback(0); break;
        case GPIO_PIN_5: Trigger_EXTI_Callback(1); break;
        case GPIO_PIN_6: Trigger_EXTI_Callback(2); break;
        case GPIO_PIN_7: Trigger_EXTI_Callback(3); break;
        default: break;
    }
}
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
    switch(GPIO_Pin)
    {
        case GPIO_PIN_4: Trigger_Rising_Callback(0); break;
        case GPIO_PIN_5: Trigger_Rising_Callback(1); break;
        case GPIO_PIN_6: Trigger_Rising_Callback(2); break;
        case GPIO_PIN_7: Trigger_Rising_Callback(3); break;
        default: break;
    }
}


 void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
 {
	     if (huart->Instance == USART2) {
        // `Size` 鏄湰娆℃帴鏀跺埌鐨勬暟鎹暱搴?
        // `rx_buffer` 涓寘鍚帴鏀跺埌鐨勬暟鎹?
				RxStart=1; 
				RxMark=Size;
			//	 if(RxStart==1)
		//		{
	
			//		RxStart=0;
			//	}
        // 鏁版嵁澶勭悊瀹屽悗锛岄噸鏂板惎鍔ㄦ帴鏀讹紝鍑嗗涓嬩竴娆℃帴鏀?
       	HAL_UARTEx_ReceiveToIdle_IT(&huart2,&rx_buf[0],64);
			}
}
 // UART 错误回调（溢出、帧错误、校验错误都进这里）
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  // 判断是否是 USART2 出错
  if(huart->Instance == USART2)
  {
    // 判断是否是【溢出错误 ORE】
    if(huart->ErrorCode & HAL_UART_ERROR_ORE)
    {
       // 溢出处理方案：
       // 1. 清除溢出标志
       __HAL_UART_CLEAR_OREFLAG(huart);

      HAL_UARTEx_ReceiveToIdle_IT(&huart2,&rx_buf[0],64);
    }
  }
}
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == htim17.Instance)
	{
			g_trig++;
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
