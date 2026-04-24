#ifndef __1002_H__
#define __1002_H__

#include "stdint.h"
#include <stdbool.h>
#include "main.h"
#include "py32f0xx_hal.h"
#include "py32f002b_hal_i2c.h"

#define STEP_WIDTH 5

#define I2C_ADDRESS 0xA0                 /* Local address 0xA0 */
#define I2C_SPEEDCLOCK 400000            /* Communication speed 400K */
#define I2C_DUTYCYCLE I2C_DUTYCYCLE_16_9 /* Duty cycle */

// ch0
#define PROXTH_CH0_CLS_REAL_VAL 1000000
#define PROXTH_CH0_FAR_REAL_VAL 1000000
// ch1
#define PROXTH_CH1_CLS_REAL_VAL 1000000
#define PROXTH_CH1_FAR_REAL_VAL 1000000
// ch2
#define PROXTH_CH2_CLS_REAL_VAL 1000000
#define PROXTH_CH2_FAR_REAL_VAL 1000000
// ch3
#define PROXTH_CH3_CLS_REAL_VAL 1000000
#define PROXTH_CH3_FAR_REAL_VAL 1000000

#define AVG_TARGET_COMP_CH0_REAL_VAL 2000000
#define AVG_TARGET_COMP_CH1_REAL_VAL 2000000
#define AVG_TARGET_COMP_CH2_REAL_VAL 2000000
#define AVG_TARGET_COMP_CH3_REAL_VAL 2000000

// 通道0
#define PROXTH_CH0_CLS (PROXTH_CH0_CLS_REAL_VAL >> 8)
#define PROXTH_CH0_FAR (PROXTH_CH0_FAR_REAL_VAL >> 8)
// 通道1
#define PROXTH_CH1_CLS (PROXTH_CH1_CLS_REAL_VAL >> 8)
#define PROXTH_CH1_FAR (PROXTH_CH1_FAR_REAL_VAL >> 8)
// 通道2
#define PROXTH_CH2_CLS (PROXTH_CH2_CLS_REAL_VAL >> 8)
#define PROXTH_CH2_FAR (PROXTH_CH2_FAR_REAL_VAL >> 8)
// 通道3
#define PROXTH_CH3_CLS (PROXTH_CH3_CLS_REAL_VAL >> 8)
#define PROXTH_CH3_FAR (PROXTH_CH3_FAR_REAL_VAL >> 8)

#define AVG_TARGET_COMP_CH0 (AVG_TARGET_COMP_CH0_REAL_VAL >> 8)
#define AVG_TARGET_COMP_CH1 (AVG_TARGET_COMP_CH2_REAL_VAL >> 8)
#define AVG_TARGET_COMP_CH2 (AVG_TARGET_COMP_CH2_REAL_VAL >> 8)
#define AVG_TARGET_COMP_CH3 (AVG_TARGET_COMP_CH2_REAL_VAL >> 8)

#define RM1X01_PROXTH_CH0_CLS 0x49
#define RM1X01_PROXTH_CH0_FAR 0x51

#define RM1X01_RAW0_CH0 0X75
#define RM1X01_USE0_CH0 0X81
#define RM1X01_AVG0_CH0 0X8D
#define RM1X01_DIF0_CH0 0X99

#define DEFAULE_I2C_ADDR_0 (0x2B) // 00101011
#define DEFAULE_I2C_ADDR_1 (0x2A) // 00101010
#define DEFAULE_I2C_ADDR_2 (0x29) // 00101001
#define DEFAULE_I2C_ADDR_3 (0x28) // 00101000

#define TX_MSG_ITERVAL_MS 10
#define INCREASE_PER 1.05f
#define DECREASE_PER 0.95f
#define IIR_ALPHA 0.6f // 低通滤波系数
#define MAX_C_CHANGE_PER_FRAME 30.0f
#define SERIALS_CAP 10.0f

void collect_1002_data(uint8_t rm1002_addr, uint8_t start_idx);
void rm1002_init(uint8_t rm1002_addr);
void read_all_rm1002_sensors(void);
void send_data_with_lfo(float *data);
void cali(int32_t *rawdata, uint8_t rm1002_addr, uint8_t start_idx);

extern volatile float c_real_time_value[];

typedef struct
{
    I2C_HandleTypeDef hi2c;
    uint8_t i2c_addr;
} i2c_handle;

typedef enum
{
    RAW_DATA,
    USE_DATA,
    AVG_DATA,
    DIF_DATA
} RM1X01_DataType;

struct RM1X01_REG_CFG
{
    uint8_t reg_address;
    uint8_t reg_value;
};

void print_1002_data(void);

void read_rmof_addres3(void);

void get_all_parastic_cap(void);

void print_parastic_cap(void);

#endif
