#ifndef __TIM_H
#define __TIM_H

#include "main.h"

extern TIM_HandleTypeDef TimHandle; // 定时器句柄

extern volatile uint8_t t_10ms_Flag;   // 10ms flag
extern volatile uint16_t t_10ms_count; // 10ms count
extern volatile uint8_t timer_coe;     // 定时器系数
extern volatile uint8_t us_delay_flag;

void APP_TimConfig(void);
void APP_ErrorHandler(void);

// void delay_100us(void);

#endif
