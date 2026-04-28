#include "I2C.h"
#include "1002.h"
#include "tim.h"

I2C_HandleTypeDef I2cHandle;

void I2C_GPIO_DeInit(void)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
}

/**
 * @brief I2C GPIO初始化
 * PB3->I2C_SCL
 * PB4->I2C_SDA
 */
void I2C_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 时钟使能 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = RM_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;        /* 输出模式 */
    GPIO_InitStruct.Pull = GPIO_NOPULL;                /*  */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; /* 高速模式 */
    HAL_GPIO_Init(RM_RST_PORT, &GPIO_InitStruct);      /* 使能 */

    GPIO_InitStruct.Pin = RM_VDD_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;   /* 输出模式 */
    GPIO_InitStruct.Pull = GPIO_NOPULL;           /*  */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  /* 低速模式 */
    HAL_GPIO_Init(RM_VDD_PORT, &GPIO_InitStruct); /* 使能 */

    // 拉高RM RST
    HAL_GPIO_WritePin(RM_RST_PORT, RM_RST_PIN, GPIO_PIN_SET);

	// 拉低RM_VDD引脚
	HAL_GPIO_WritePin(RM_VDD_PORT, RM_VDD_PIN, GPIO_PIN_RESET);

	HAL_Delay(200);

	HAL_GPIO_WritePin(RM_VDD_PORT, RM_VDD_PIN, GPIO_PIN_SET);


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
