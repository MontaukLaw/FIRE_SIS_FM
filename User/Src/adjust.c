#include "adjust.h"

volatile uint8_t   RM1002B_X              = 0;     // 锐盟地址
volatile uint8_t G_Adjustment_Flag        = 1;     // 让调整部分只运行一次
volatile float c_value[16]                = {0.0}; // 传感器值
volatile uint8_t G_Printf_Flag            = 0;     // 10ms打印一次
float *c_read_value;                               // 传感器值
const uint8_t I2C_ADDRS[4] = {DEFAULE_I2C_ADDR_0, DEFAULE_I2C_ADDR_1, DEFAULE_I2C_ADDR_2, DEFAULE_I2C_ADDR_3};// RM地址

// 调整寄生电容值
void calibrate_channel(uint8_t channel) 
{
    // if (t_10ms_Flag == 1) 
    // {
    //     t_10ms_Flag = 0;
    //     for (int rm = 0; rm < 4; rm++) 
    //     {
    //         RM1002B_X = I2C_ADDRS[rm];
    //         c_read_value = collect_1002_data(RM1002B_X); // 计算电容
    //         for (int i = 0; i < 4; i++) 
    //         {
    //             c_value[i + rm * 4] = c_read_value[i];
    //         }
    //     }
    //     G_Printf_Flag = 1;
    // }

    // if (G_Adjustment_Flag && c_value[channel] != prev_value) 
    // {
    //     if (c_value[channel] > 0 && c_value[channel] < 40.330048) {
    //         cxxx0[channel] -= 0.8;
    //     } else if (c_value[channel] > 40.330048 && c_value[channel] < 50.330048) {
    //         cxxx0[channel] -= 0.3;
    //     } else if (c_value[channel] > 50.330048 && c_value[channel] < 53.330048) {
    //         cxxx0[channel] -= 0.1;
    //     } else if (c_value[channel] > 53.330048 && c_value[channel] < 55.000048) {
    //         cxxx0[channel] -= 0.01;
    //     } else if (c_value[channel] > 55.000048 && c_value[channel] < 55.330048) {
    //         cxxx0[channel] -= 0.001;
    //     } else if (c_value[channel] > 70.330048 && c_value[channel] < 200.330048) {
    //         cxxx0[channel] += 0.8;
    //     } else if (c_value[channel] > 60.330048 && c_value[channel] < 70.330048) {
    //         cxxx0[channel] += 0.3;
    //     } else if (c_value[channel] > 57.330048 && c_value[channel] < 60.330048) {
    //         cxxx0[channel] += 0.1;
    //     } else if (c_value[channel] > 55.321217 && c_value[channel] < 57.330048) {
    //         cxxx0[channel] += 0.01;
    //     } else if (c_value[channel] > 55.330048 && c_value[channel] < 55.660048) {
    //         cxxx0[channel] += 0.001;
    //     }
    //     prev_value = c_value[channel];
    //     G_Adjustment_Flag = 1;
    // }

    // if (c_value[channel] >= 55.321217 && c_value[channel] <= 55.330048) {
    //     G_Adjustment_Flag = 0;
    //     printf("CH%d success!!!!!! %f, %f\r\n", channel, c_value[channel], cxxx0[channel]);
    //     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, GPIO_PIN_RESET); // G 亮
    // }
}










