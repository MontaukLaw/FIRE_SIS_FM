#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "py32f0xx_hal.h"
#include "py32f002bxx_Start_Kit.h"

#include <string.h>
#include "math.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "user_define.h"
#include "user_struct.h"

#include "1002.h"
#include "usart.h"
#include "adjust.h"
#include "led.h"
#include "tim.h"
#include "iwdg.h"
#include "flash.h"
#include "arithmetic.h"

#include "sys.h"
#include "app_usart.h"

#include "voice_app.h"
#include "baseline_tracking.h"
#include "user_define.h"

#include "sc7a20.h"
#include "1002.h"
#include "sleep.h"
#include "mh1612s.h"
#include "py32f002b_hal_spi.h"
#include "picc.h"
#include "card_emu.h"
#include "nfc_app.h"
#include "nfc_config.h"
#include "rfid.h"
#include "mifare.h"
#include "nfc_transfer.h"
#include "nfc_debug.h"
#include "iso14443a.h"
#include "iso14443b.h"
#include "intc.h"
#include "user_refine.h"
#include "sim_i2c.h"
#include "gpio_i2c.h"

#define DEFAULE_I2C_ADDR_0  (0x2B) /* 锐盟地址 */
#define DEFAULE_I2C_ADDR_1  (0x2A) 
#define DEFAULE_I2C_ADDR_2  (0x29) 
#define DEFAULE_I2C_ADDR_3  (0x28) 

#define FLEX_FLAG  0
#define SENSOR_NUM 4 // 16

extern uint8_t Generic_Reply[50];
extern uint8_t UDP_11 [116];
extern uint8_t UDP_15 [20];
extern TIM_HandleTypeDef  TimHandle;       // 定时器句柄
extern IWDG_HandleTypeDef IwdgHandle;      // 看门狗句柄
extern uint32_t G_USART1_RX_Count;         // 串口计数
extern uint8_t  G_USART1_RX_Buffer[64];    // 串口缓冲区
extern volatile uint16_t  G_setFlag;       // 串口标记位
extern volatile uint8_t   G_USART1_RX_Flag;// 空闲中断标记;
extern volatile uint8_t RM1002B_X;         // RM地址
extern volatile float c_value[16];         // 传感器值
extern float *c_read_value;                // 电容值
extern float cxxx0[4];                     // RM0寄生电容值
extern float cxxx1[4];                     // RM1寄生电容值
extern float cxxx2[4];                     // RM2寄生电容值
extern float cxxx3[4];                     // RM3寄生电容值

extern uint32_t exti_triggered_ts;
extern uint32_t last_active_ts;

#endif /* __MAIN_H */
