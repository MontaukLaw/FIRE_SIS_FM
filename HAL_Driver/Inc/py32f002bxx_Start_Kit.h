/**
  ******************************************************************************
  * @file    py32f002bxx_Start_Kit.h
  * @author  MCU Application Team
  * @brief   This file provides set of firmware functions to manage Leds, 
  *          push-button available on Start Kit.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef PY32F002B_START_KIT_H
#define PY32F002B_START_KIT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_hal.h"

/** @addtogroup BSP
  * @{
  */

/** @defgroup py32f002b_Start_Kit
  * @brief This section contains the exported types, contants and functions
  *        required to use the Nucleo 32 board.
  * @{
  */

/** @defgroup py32f002b_Start_Kit_Exported_Types Exported Types
  * @{
  */
typedef enum
{
  LED3 = 0,
  LED_GREEN = LED3,
    
    LED_G = 0,
    LED1_R = 1,
    LED1_G = 2,
    LED1_B = 3,
    LED2_R = 4,
    LED2_G = 5,
    LED2_B = 6,
    LED_OFF = 7
} Led_TypeDef;

typedef enum
{
  BUTTON_USER = 0,
  /* Alias */
  BUTTON_KEY  = BUTTON_USER
} Button_TypeDef;

typedef enum
{
  BUTTON_MODE_GPIO = 0,
  BUTTON_MODE_EXTI = 1
} ButtonMode_TypeDef;

#define LEDn                               8

#define LED3_PIN                           GPIO_PIN_3
#define LED3_GPIO_PORT                     GPIOA

#define LED_G_PIN                           GPIO_PIN_7
#define LED_G_GPIO_PORT                     GPIOA
#define LED1_R_PIN                           GPIO_PIN_1
#define LED1_R_GPIO_PORT                     GPIOB
#define LED1_G_PIN                           GPIO_PIN_5
#define LED1_G_GPIO_PORT                     GPIOB
#define LED1_B_PIN                           GPIO_PIN_2
#define LED1_B_GPIO_PORT                     GPIOB
#define LED2_R_PIN                           GPIO_PIN_7
#define LED2_R_GPIO_PORT                     GPIOB
#define LED2_G_PIN                           GPIO_PIN_5
#define LED2_G_GPIO_PORT                     GPIOB
#define LED2_B_PIN                           GPIO_PIN_1
#define LED2_B_GPIO_PORT                     GPIOC
#define LED_OFF_PIN                           GPIO_PIN_8
#define LED_OFF_GPIO_PORT                     GPIOA


#define LEDA_GPIO_CLK_ENABLE()             __HAL_RCC_GPIOA_CLK_ENABLE()
#define LEDB_GPIO_CLK_ENABLE()             __HAL_RCC_GPIOB_CLK_ENABLE()
#define LEDC_GPIO_CLK_ENABLE()             __HAL_RCC_GPIOC_CLK_ENABLE()

#define LED3_GPIO_CLK_DISABLE()            __HAL_RCC_GPIOA_CLK_DISABLE()



#define BUTTONn                            1

/**
  * @brief User push-button
  */
#define USER_BUTTON_PIN                         GPIO_PIN_4
#define USER_BUTTON_GPIO_PORT                   GPIOA
#define USER_BUTTON_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOA_CLK_ENABLE()
#define USER_BUTTON_GPIO_CLK_DISABLE()          __HAL_RCC_GPIOA_CLK_DISABLE()
#define USER_BUTTON_EXTI_IRQn                   EXTI0_1_IRQn

/* Aliases */
#define KEY_BUTTON_PIN                          USER_BUTTON_PIN
#define KEY_BUTTON_GPIO_PORT                    USER_BUTTON_GPIO_PORT
#define KEY_BUTTON_GPIO_CLK_ENABLE()            USER_BUTTON_GPIO_CLK_ENABLE()
#define KEY_BUTTON_GPIO_CLK_DISABLE()           USER_BUTTON_GPIO_CLK_DISABLE()
#define KEY_BUTTON_EXTI_IRQn                    USER_BUTTON_EXTI_IRQn

#define BUTTONx_GPIO_CLK_ENABLE(__INDEX__)      do { if((__INDEX__) == 0) USER_BUTTON_GPIO_CLK_ENABLE();} while(0)
#define BUTTONx_GPIO_CLK_DISABLE(__INDEX__)     (((__INDEX__) == 0) ? USER_BUTTON_GPIO_CLK_DISABLE() : 0)


#ifdef HAL_UART_MODULE_ENABLED
//debug printf redirect config
#define DEBUG_USART_BAUDRATE                    115200

#define DEBUG_USART                             USART1
#define DEBUG_USART_CLK_ENABLE()                do { \
                                                     __IO uint32_t tmpreg = 0x00U; \
                                                     SET_BIT(RCC->APBENR2, RCC_APBENR2_USART1EN);\
                                                     /* Delay after an RCC peripheral clock enabling */ \
                                                     tmpreg = READ_BIT(RCC->APBENR2, RCC_APBENR2_USART1EN);\
                                                     UNUSED(tmpreg); \
                                                   } while(0U)

#define __GPIOB_CLK_ENABLE()                    do { \
                                                     __IO uint32_t tmpreg = 0x00U; \
                                                     SET_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN);\
                                                     /* Delay after an RCC peripheral clock enabling */ \
                                                     tmpreg = READ_BIT(RCC->IOPENR, RCC_IOPENR_GPIOBEN);\
                                                     UNUSED(tmpreg); \
                                                   } while(0U)

#define DEBUG_USART_RX_GPIO_PORT                GPIOA
#define DEBUG_USART_RX_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOA_CLK_ENABLE()
#define DEBUG_USART_RX_PIN                      GPIO_PIN_4
#define DEBUG_USART_RX_AF                       GPIO_AF1_USART1

#define DEBUG_USART_TX_GPIO_PORT                GPIOA
#define DEBUG_USART_TX_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOA_CLK_ENABLE()
#define DEBUG_USART_TX_PIN                      GPIO_PIN_3
#define DEBUG_USART_TX_AF                       GPIO_AF1_USART1

#define DEBUG_USART_IRQHandler                  USART1_IRQHandler
#define DEBUG_USART_IRQ                         USART1_IRQn
#endif
/************************************************************/

#ifdef HAL_UART_MODULE_ENABLED
extern UART_HandleTypeDef DebugUartHandle;
#endif

/** @defgroup Functions
  * @{
  */
uint32_t         BSP_GetVersion(void);

void             BSP_LED_Init(void);
void             RM1002_RST_Init(void);
void             BSP_RM1002B_1_On(void);
void             BSP_RM1002B_1_Off(void);
void             BSP_RM1002B_2_On(void);
void             BSP_RM1002B_2_Off(void);

void             BSP_LED_DeInit(Led_TypeDef Led);
void             BSP_LED_On(Led_TypeDef Led);
void             BSP_LED_Off(Led_TypeDef Led);
void             BSP_LED_Toggle(Led_TypeDef Led);

void             BSP_PB_Init(Button_TypeDef Button, ButtonMode_TypeDef ButtonMode);
void             BSP_PB_DeInit(Button_TypeDef Button);
uint32_t         BSP_PB_GetState(Button_TypeDef Button);

void             BSP_USART_Config(void);
#define          DEBUG_USART_Config       BSP_USART_Config
/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* PY32F002B_START_KIT_H */

/************************ (C) COPYRIGHT Puya *****END OF FILE****/
