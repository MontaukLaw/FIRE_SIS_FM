#ifndef __FLASH_H
#define __FLASH_H

#include "usart.h"
#include "main.h"
#include "py32f0xx_hal.h"
#include "py32f002b_hal_flash.h"

#define FLASH_CAP_ADDR    0x08005000 // 寄生电容存储起始地址 存储地址到0x0800507E
#define FLASH_COE_ADDR    0x08005080 // 归一化系数存储起始地址 存储地址到0x080050FF
#define FLASH_STATUS_ADDR 0x08005100 // 状态存储起始地址
#define FLASH_TIMER_ADDR  0x08005180 // 定时器系数存储起始地址

#define MCU_STATUS_INIT 0x02 // 初始化状态

extern float cap_buffer[16];
extern float coe_buffer[16];
extern uint8_t MCU_Status;

void Flash_Write(uint32_t flash_addr, uint32_t buffer_size, uint32_t* data_src) ;
void Flash_Read(uint32_t flash_addr, uint32_t buffer_size, uint32_t* data_src);
void Flash_Write_Status(uint32_t flash_addr, uint8_t status);
uint8_t Flash_Read_Status(uint32_t flash_addr);
void Flash_Init(void);
void Flash_Print(void);

#endif

