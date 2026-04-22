#ifndef __USART_H
#define __USART_H

#include "py32f0xx_hal.h"
#include "app_usart.h"


extern UART_HandleTypeDef UartHandle; // 串口句柄


// 引用数组必须写清楚数组大小
extern uint32_t G_USART1_RX_Count;         // 串口计数
extern uint8_t  G_USART1_RX_Buffer[64];    // 串口缓冲区
extern volatile uint16_t  G_setFlag;       // 串口标记位
extern volatile uint8_t   G_USART1_RX_Flag;// 空闲中断标记;

// 函数定义
void USART1_GPIO_Init(void);
void USART1_Init(uint32_t baudrate);
void USART1_Send_Byte(uint8_t dat);
void USART1_Send_String(char *str);
void USART1_Send_Hex8(uint8_t value);

void uart_send_data(float *data);

void uart_send_multi_data(void);

void uart_send_float_data(float data);

void uart_send_gsensor_axes(int16_t x, int16_t y, int16_t z, int16_t dx, int16_t dy, int16_t dz);
void uart_send_gsensor_event(uint8_t success, uint8_t fail_mask, int16_t peak_any, uint16_t energy, uint8_t active_samples, uint8_t rebound_detected);

#endif
