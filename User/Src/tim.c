#include "tim.h"

TIM_HandleTypeDef TimHandle; // 定时器句柄

volatile uint8_t t_10ms_Flag = 0;   // 10ms flag
volatile uint16_t t_10ms_count = 0; // 10ms count
volatile uint8_t timer_coe = 0;     // 定时器系数

// volatile uint8_t us_counter = 0;
// volatile uint8_t us_delay_flag = 0;
// volatile uint8_t if_timer_start = 0;

volatile uint8_t delay_tick = 0;

// TIM初始化
// 239对应10uS
void APP_TimConfig(void)
{
    TimHandle.Instance = TIM1;
    TimHandle.Init.Period = 1 - 1;
    TimHandle.Init.Prescaler = 239 - 1;
    TimHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TimHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    TimHandle.Init.RepetitionCounter = 1 - 1;
    TimHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&TimHandle) != HAL_OK)
    {
        APP_ErrorHandler();
    }

    HAL_TIM_Base_Start_IT(&TimHandle);
}

// TIM回调函数
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (delay_tick > 0)
    {
        delay_tick--;
    }
}

void delay_100us(void)
{
    __disable_irq();
    delay_tick = 10; // 10 * 10us
    __enable_irq();

    while (delay_tick > 0)
    {
    }
}

// 错误处理函数
void APP_ErrorHandler(void)
{
    while (1)
    {
    }
}
