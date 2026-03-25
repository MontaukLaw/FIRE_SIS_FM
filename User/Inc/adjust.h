#ifndef __ADJUST_H
#define __ADJUST_H

#include "main.h"
#include "py32f0xx_hal.h"

extern volatile uint8_t G_Adjustment_Flag; // 让调整部分只运行一次
extern volatile uint8_t   RM1002B_X;       // RM地址
extern float *c_read_value;                // 电容值
extern volatile float c_value[16];         // 传感器值
extern volatile uint8_t G_Printf_Flag;     // 20ms打印一次
extern float cxxx0[4];                     // RM0寄生电容值
extern float cxxx1[4];                     // RM1寄生电容值
extern float cxxx2[4];                     // RM2寄生电容值
extern float cxxx3[4];                     // RM3寄生电容值

void calibrate_channel(uint8_t channel);

#endif
