#ifndef __SYS_H
#define __SYS_H

#define RCC_SYSCLKSOURCE_HSI 0x00000000U /*!< HSI selection as system clock */

#include "py32f0xx_hal.h"
#include "iwdg.h"
#include "usart.h"
#include "tim.h"
#include "I2C.h"

#define DEBUG_LOG_ENABLE 0

void app_config(void);
void System_Clock_Config_HSI_24Mhz(void);
void RM_Init(void);
void Array_Write_Init(void);
void delay_100us(void);

float ema_float(float last, float now, uint16_t alpha_num, uint16_t alpha_den);

float float_asb_diff(float a, float b);

float low_filter_op(float pre_val, float new_val, float *data_ptr, float alpha);

int is_timeout(tick start_time, tick interval);

tick get_diff_tick(tick cur_tick, tick prior_tick);

void APP_Config_Without_UART(void);

void rm_init(void);

void show_running(void);

#endif
