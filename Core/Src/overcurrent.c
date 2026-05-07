/**
 * @file overcurrent.c - Over-current protection via ADC + shunt resistor
 * @brief PA0 (ADC0) reads voltage across R24 (50mΩ shunt)
 *        Op-amp LMV321 amplifies the shunt voltage
 *
 *  Protection logic:
 *    - If current exceeds threshold → disable all PWM outputs
 *    - Display "Err" on 7-segment
 *    - Wait for button press or power cycle to restart
 *
 *  Threshold calculation:
 *    Shunt = 50mΩ, op-amp gain depends on resistor network
 *    Assume gain = (R17+R16)/R16 = (100k+10k)/10k = 11
 *    At 3A: Vshunt = 3 × 0.05 = 0.15V, Vout = 0.15 × 11 = 1.65V
 *    ADC reading at 3.3V ref, 12-bit: 1.65/3.3 × 4095 = 2047
 *    Set threshold slightly above: ~2047 (3A per channel, 4ch = 12A)
 *
 *  Note: Adjust gain and threshold based on actual hardware measurement
 */
#include "main.h"

ADC_HandleTypeDef hadc1;
static bool fault_state = false;
static uint32_t fault_count = 0;

#define OC_THRESHOLD   2000U   /* ADC value — adjust per hardware */
#define OC_CONSECUTIVE  5U      /* consecutive readings to trip */

void Overcurrent_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA0 as analog input */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4; /* 16 MHz */
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.LowPowerAutoPowerOff  = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode      = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
        Error_Handler();

    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_39CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
        Error_Handler();

    fault_state = false;
    fault_count = 0;
}

void Overcurrent_Process(void)
{
    if (fault_state) {
        /* Fault mode — all outputs off, wait for reset
         * (button or power cycle) */
        return;
    }

    /* Start ADC conversion */
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK) {
        uint32_t adc_val = HAL_ADC_GetValue(&hadc1);

        if (adc_val >= OC_THRESHOLD) {
            fault_count++;
            if (fault_count >= OC_CONSECUTIVE) {
                /* OVERCURRENT FAULT */
                fault_state = true;

                /* Disable all outputs */
                for (int ch = 0; ch < 4; ch++)
                    Set_PWM_Duty(ch, 0);

                /* Show "Err" on display */
                /* This is a simplified display — ideally use CH455G */
                HAL_GPIO_WritePin(LED_GPIO, LED_B_PIN, GPIO_PIN_RESET);
            }
        } else {
            fault_count = 0;
        }
    }
    HAL_ADC_Stop(&hadc1);
}
