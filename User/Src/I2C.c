#include "I2C.h"
#include "1002.h"
#include "tim.h"

I2C_HandleTypeDef I2cHandle;

/**
 * @brief I2C GPIO初始化
 * PB3->I2C_SCL
 * PB4->I2C_SDA
 */
void I2C_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
}

/**
 * @brief I2C初始化配置
 * @param i2c_addr I2C从机地址（7位地址，函数内部会自动左移1位）
 */
void I2C_Init(void)
{
    /* I2C initialization */
    I2cHandle.Instance = I2C;                                 /* I2C */
    I2cHandle.Init.ClockSpeed = I2C_SPEEDCLOCK;               /* I2C communication speed */
    I2cHandle.Init.DutyCycle = I2C_DUTYCYCLE;                 /* I2C duty cycle */
    I2cHandle.Init.OwnAddress1 = 0;                           /* I2C address */
    I2cHandle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; /* Prohibit broadcast calls */
    I2cHandle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;     /* Allow clock extension */
    if (HAL_I2C_Init(&I2cHandle) != HAL_OK)
    {
        APP_ErrorHandler();
    }
}
