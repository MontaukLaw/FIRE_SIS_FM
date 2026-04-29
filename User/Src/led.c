#include "led.h"
#include "user_define.h"

void APP_GPIOInit(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* 时钟使能 */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = VOICE_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;		   /* 输出模式 */
	GPIO_InitStruct.Pull = GPIO_NOPULL;				   /* 使能上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; /* 高速模式 */
	HAL_GPIO_Init(VOICE_PORT, &GPIO_InitStruct);	   /* 使能 */

	// SC7A20_VDD引脚
	GPIO_InitStruct.Pin = SC7A20_VDD_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;		  /* 输出模式 */
	GPIO_InitStruct.Pull = GPIO_NOPULL;				  /* 使能上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;	  /* 低速模式 */
	HAL_GPIO_Init(SC7A20_VDD_PORT, &GPIO_InitStruct); /* 使能 */

	// 拉低voice引脚
	HAL_GPIO_WritePin(VOICE_PORT, VOICE_PIN, GPIO_PIN_RESET);

	// 拉高接通SC7A20_VDD引脚
	// HAL_GPIO_WritePin(SC7A20_VDD_PORT, SC7A20_VDD_PIN, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(SC7A20_VDD_PORT, SC7A20_VDD_PIN, GPIO_PIN_SET);

	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;		   /* 输出模式 */
	GPIO_InitStruct.Pull = GPIO_NOPULL;				   /* 使能上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; /* 高速模式 */
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);	   /* 使能 */
}

/**
 * @brief LED初始化
 * PC1->LED
 */
void APP_GPIOInit_(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/* 时钟使能 */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* PA0 PA1 PA5 PA6 */
	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 | GPIO_PIN_6;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;		   /* 输出模式 */
	GPIO_InitStruct.Pull = GPIO_PULLUP;				   /* 使能上拉 */
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; /* 高速模式 */
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);			   /* 使能 */

	/* PB0 */
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	// RM RST引脚全部拉低
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

	// 灯拉高
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_SET);
}
