#ifndef __APP_USART_H
#define __APP_USART_H

#include "py32f0xx_hal.h"
#include "usart.h"
#include "main.h"

#define SENSOR_NUM_LENGTH 6

#define UART_RX_REPLY_20 20
#define UART_RX_DATA_LENGTH 116
#define Reply_72_LENGTH 28 // 71命令单个传感器数据长度

// 定义帧头和协议相关的固定内容
#define FRAME_HEADER           0xFF, 0xFF, 0x06, 0x09 // 帧头       0-3
#define FRAME_NUMBER           0x00, 0x00             // 帧序列号   4-5
#define FRAME_LENGTH           0x00, 0x00             // 帧长度     6-7
#define DEVICE_ID              0x33, 0x30, 0x0F, 0x0F // 设备 ID    8-11
#define TARGET_ID              0x12, 0x34, 0x56, 0x78 // 目标 ID    12-15
#define RESPONSE_CODE          0x00, 0x00             // 回复       16-17
#define STATUS_CODE            0x00, 0x01             // 状态字     18-19  53 59 43 33 33 33 33 2d 46 46
#define SYC_STRING             0x53, 0x59, 0x43, 0x33, 0x33, 0x33, 0x33, 0x2D, 0x46, 0x46 // SYC3333-FF     20-29
#define RESERVED_SPACE         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // 保留空间      30-49


extern volatile uint8_t UART_RX_CMD;
extern volatile uint8_t UART_RX_CMD_11;
extern volatile uint8_t UART_RX_CMD_13;
extern volatile uint8_t UART_RX_CMD_15;
extern volatile uint8_t UART_RX_CMD_71;
extern volatile uint8_t UART_RX_CMD_99;


extern uint8_t Generic_Reply[];
extern uint8_t UDP_11 [];
extern uint8_t UDP_15 [];


void process_value(float value, uint8_t *udp_array, int start_index);
void APP_Clear_Buffer(void);
void APP_Protocol(void);
void APP_Command(void);
void APP_Command_1(void);
void APP_Command_3(void);
void APP_Command_5(void);
void APP_Command_7(void);
void APP_Command_13(void);
void APP_Command_15(void);
void APP_Command_71(void);
void APP_Command_99(void);
void APP_Command_AA(void);

#endif
