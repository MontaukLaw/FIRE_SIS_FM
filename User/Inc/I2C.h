#ifndef __I2C_H
#define __I2C_H


#include "py32f0xx_hal.h"
#include "py32f002b_hal_i2c.h"

extern I2C_HandleTypeDef I2cHandle;

void I2C_GPIO_Init(void);
// void I2C_Init(uint8_t i2c_addr);

void I2C_Init(void);

#endif
