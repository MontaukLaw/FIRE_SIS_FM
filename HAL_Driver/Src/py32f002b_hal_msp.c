/**
  ******************************************************************************
  * @file    py32f002b_hal_msp.c
  * @author  MCU Application Team
  * @brief   This file provides code for the MSP Initialization
  *          and de-Initialization codes.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) Puya Semiconductor Co.
  * All rights reserved.</center></h2>
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* External functions --------------------------------------------------------*/

/**
  * @brief Initialize global MSP
  */
void HAL_MspInit(void)
{
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
}

/**
  * @brief Initialize ADC MSP
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_ADC_CLK_ENABLE();

  HAL_NVIC_SetPriority(ADC_COMP_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(ADC_COMP_IRQn);
}
/**
  * @brief Initialize MSP for I2C.
  */

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_I2C_CLK_ENABLE();                                 /* I2C clock enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();                               /* GPIOB clock enable */

  /**I2C pin configuration
  PB03     ------> I2C1_SCL
  PB04     ------> I2C1_SDA
  */
  GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;                     /* open drain */
  GPIO_InitStruct.Pull = GPIO_PULLUP;                         /* pull up */
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;          
  GPIO_InitStruct.Alternate = GPIO_AF6_I2C1;                   /* Reuse as I2C */
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);                     /* GPIO initialization */
  
  /*Reset I2C*/
  __HAL_RCC_I2C_FORCE_RESET();
  __HAL_RCC_I2C_RELEASE_RESET();
}

/**
  * @brief Initialize TIM MSP
  */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
  /* Enable TIM1 Clock */
  __HAL_RCC_TIM1_CLK_ENABLE(); 
 
  /* Interrupt Configuration */
  NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
  NVIC_SetPriority(TIM1_BRK_UP_TRG_COM_IRQn,0);
}

/**
  * @brief Initialize MSP for USART.
  * @param huart：Initialize the handle of USART
  */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef  GPIO_InitStruct;// GPIO句柄

    // 时钟使能
    __HAL_RCC_GPIOA_CLK_ENABLE();   // 使能IO时钟
    __HAL_RCC_USART1_CLK_ENABLE();  // 使能串口时钟

    /* 
        GPIO 初始化
        PA3：TX
        PA4：RX
    */
    GPIO_InitStruct.Pin       = GPIO_PIN_3;             // 引脚
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;        // 推挽式复用（硬件图无外部上拉电阻）
    GPIO_InitStruct.Pull      = GPIO_PULLUP;            // 无上拉下拉
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;   // 高速
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;        // 要连接到所选引脚的外设
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);             // 初始化引脚配置

    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
/************************ (C) COPYRIGHT Puya *****END OF FILE******************/
