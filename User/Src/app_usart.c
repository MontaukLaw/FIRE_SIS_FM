#include "app_usart.h"

volatile uint8_t UART_RX_CMD = 0; // 协议命令字
volatile uint8_t UART_RX_CMD_11 = 0; // 发送命令
volatile uint8_t UART_RX_CMD_13 = 0; // 停止发送命令

/* 协议发送-------------------------------------------------------------------*/
uint8_t Generic_Reply[50] = {
    FRAME_HEADER          // 帧头
    ,FRAME_NUMBER         // 帧序号
    ,FRAME_LENGTH         // 帧长
    ,DEVICE_ID            // 设备ID
    ,TARGET_ID            // 目标ID
    ,RESPONSE_CODE        // 命令字
    ,STATUS_CODE          // 数据
    ,SYC_STRING           // 设备类型
    ,RESERVED_SPACE       // 保留
};


uint8_t UDP_11[116] = {
    FRAME_HEADER                                //0,1,2,3         固定：帧头
    ,0x00,0x00                                  //4,5             变量：帧序列号
    ,0x00,0x6C                                  //6,7             固定：帧长
    ,DEVICE_ID                                  //8,9,10,11       固定：设备ID
    ,TARGET_ID                                  //12,13,14,15     固定：目标ID
    ,0x00,0x12                                  //16,17           固定：命令字
    ,0x00,0x10                                  //18,19           固定：状态字---传感器个数
    /* RM0 */
    ,0x00,0x00,0x00,0x00,0x00,0x00              //20,21,22,23,24,25
    ,0x00,0x00,0x00,0x00,0x00,0x00              //26,27,28,29,30,31
    ,0x00,0x00,0x00,0x00,0x00,0x00              //32,33,34,35,36,37
    ,0x00,0x00,0x00,0x00,0x00,0x00              //38,39,40,41,42,43
    /* RM1 */
    ,0x00,0x00,0x00,0x00,0x00,0x00              //44,45,46,47,48,49
    ,0x00,0x00,0x00,0x00,0x00,0x00              //50,51,52,53,54,55
    ,0x00,0x00,0x00,0x00,0x00,0x00              //56,57,58,59,60,61
    ,0x00,0x00,0x00,0x00,0x00,0x00              //62,63,64,65,66,67
    /* RM2 */
    ,0x00,0x00,0x00,0x00,0x00,0x00              //68,69,70,71,72,73
    ,0x00,0x00,0x00,0x00,0x00,0x00              //74,75,76,77,78,79
    ,0x00,0x00,0x00,0x00,0x00,0x00              //80,81,82,83,84,85
    ,0x00,0x00,0x00,0x00,0x00,0x00              //86,87,88,89,90,91
    /* RM3 */
    ,0x00,0x00,0x00,0x00,0x00,0x00              //92,93,94,95,96,97
    ,0x00,0x00,0x00,0x00,0x00,0x00              //98,99,100,101,102,103
    ,0x00,0x00,0x00,0x00,0x00,0x00              //104,105,106,107,108,109
    ,0x00,0x00,0x00,0x00,0x00,0x00              //110,111,112,113,114,115
};


/**
 * @brief 浮点数转换成十六进制
 * 
 * @param value 浮点数
 * @param udp_array 十六进制数组
 * @param start_index 起始索引
 */
void process_value(float value, uint8_t *udp_array, int start_index) 
{
    int int_part = (int)value; // 整数部分
    float flo_part = value - int_part; // 小数部分
    unsigned long int flo_to_int = (unsigned long int)(flo_part * 1000000); // 放大成整数处理

    udp_array[start_index]     = (int_part   & 0xFF0000) >> 16; // 整数部分
    udp_array[start_index + 1] = (int_part   & 0x00FF00) >> 8;
    udp_array[start_index + 2] = (int_part   & 0x0000FF);
    udp_array[start_index + 3] = (flo_to_int & 0xFF0000) >> 16; // 小数部分
    udp_array[start_index + 4] = (flo_to_int & 0x00FF00) >> 8;
    udp_array[start_index + 5] = (flo_to_int & 0x0000FF);
}


/**
 * @brief 清除串口缓冲区
 * 
 */
void APP_Clear_Buffer(void)
{
    memset(G_USART1_RX_Buffer, 0, sizeof(G_USART1_RX_Buffer)); // 清空接收缓冲区
    G_USART1_RX_Count = 0; // 清除接收计数器
    G_USART1_RX_Flag  = 0; // 清除空闲中断标记
}


/**
 * @brief 解包函数
 * 
 */
void APP_Protocol(void)
{
    uint16_t USART1_RX_Len = sizeof((char *)G_USART1_RX_Buffer); // 计算接收到的数据长度
    if(USART1_RX_Len > 0)
    {
        if (G_USART1_RX_Buffer[0] == 0x23 && G_USART1_RX_Buffer[1] == 0x24)
        {
            // 根据命令字设置标志位
            switch(G_USART1_RX_Buffer[2])
            {
                case 0x01:
                    UART_RX_CMD = 0xFF; // 打印寄生电容
                    break;
            }
        }
        if (G_USART1_RX_Buffer[0] == 0xFF && G_USART1_RX_Buffer[1] == 0xFF && 
            G_USART1_RX_Buffer[2] == 0x06 && G_USART1_RX_Buffer[3] == 0x09)
        {
            // 处理第二种数据包
            UART_RX_CMD = G_USART1_RX_Buffer[17];
        }
    }
}


/**
 * @brief 处理命令
 * 
 */
void APP_Command(void)
{
    switch(UART_RX_CMD)
    {
        case 0x01:
            APP_Command_1();
            break;
        case 0x03:
            APP_Command_3();
            break;
        case 0x05:
            APP_Command_5();
            break;
        case 0x07:
            APP_Command_7();
            break;
        case 0x11:
            UART_RX_CMD_11 = 1;
            break;
        case 0x13:
            UART_RX_CMD_11 = 0;
            UART_RX_CMD_13 = 1;
            APP_Command_13();
            break;
        case 0x15:
            APP_Command_15();
            break;   
        case 0x71:
            APP_Command_71();
            break;
        case 0x99:
            APP_Command_99();
            break;
        case 0xAA:
            APP_Command_AA();
            break;
    }
    APP_Clear_Buffer(); // 清除串口缓冲区
}


/**
 * @brief 处理1命令
 * 设备查询命令
 */
void APP_Command_1(void)
{
    UART_RX_CMD = 0;
    Generic_Reply[5] = 0x01;  // 设置帧号
    Generic_Reply[7] = 0x2A;  // 设置帧长
    Generic_Reply[17] = 0x02; // 设置回复

    HAL_UART_Transmit(&UartHandle, (uint8_t *)Generic_Reply, sizeof(Generic_Reply), 20); // 串口发送数据
    while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成
}


/**
 * @brief 处理3命令
 * 传感器初始化命令
 */
void APP_Command_3(void)
{
    UART_RX_CMD = 0;
    UDP_11[17] = 0x04;// 设置帧号

    HAL_UART_Transmit(&UartHandle, (uint8_t *)UDP_11, sizeof(UDP_11), 20); // 串口发送数据
    while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成
}


/**
 * @brief 处理5命令
 * 设备序列号读取命令
 */
void APP_Command_5(void)
{
    UART_RX_CMD = 0;
    Generic_Reply[5] = 0x06;  // 设置帧号
    Generic_Reply[7] = 0x0c;  // 设置帧长 12+8
    Generic_Reply[17] = 0x06; // 设置回复

    HAL_UART_Transmit(&UartHandle, (uint8_t *)Generic_Reply, UART_RX_REPLY_20, 20); // 串口发送数据
    while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成
}


/**
 * @brief 处理7命令
 * 设备版本号读取
 */
void APP_Command_7(void)
{
    UART_RX_CMD = 0;
    Generic_Reply[5] = 0x07;  // 设置帧号
    Generic_Reply[7] = 0x0c;  // 设置帧长 12+8
    Generic_Reply[17] = 0x08; // 设置回复

    HAL_UART_Transmit(&UartHandle, (uint8_t *)Generic_Reply, UART_RX_REPLY_20, 20); // 串口发送数据
    while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成
}

/**
 * @brief 处理13命令
 * 
 */
void APP_Command_13(void)
{
    UART_RX_CMD_11 = 0;
    UART_RX_CMD_13 = 0;
    Generic_Reply[5] = 0x0E;  // 设置帧号
    Generic_Reply[7] = 0x2a;  // 设置帧长
    Generic_Reply[17] = 0x14; // 设置回复

    // 回复指令
    HAL_UART_Transmit(&UartHandle, (uint8_t *)Generic_Reply, UART_RX_REPLY_20, 20); // 串口发送数据
    while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成
}


/**
 * @brief 处理15命令
 * 数据长度读取命令
 */
void APP_Command_15(void)
{
    UART_RX_CMD = 0;

    uint8_t UDP_15[20] = {
        FRAME_HEADER
        ,0x00,0x01
        ,0x00,0x0c
        ,DEVICE_ID
        ,TARGET_ID
        ,0x00,0x16
        ,0x00,0x03
    };

    HAL_UART_Transmit(&UartHandle, (uint8_t *)UDP_15, sizeof(UDP_15), 20); // 串口发送数据
    while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成
}


/**
 * @brief 处理71命令
 * 
 */
void APP_Command_71(void)
{
    UART_RX_CMD = 0;

    uint8_t Reply_72[118] = {
        FRAME_HEADER  // 0-3   帧头
        ,0x00,0x00    // 4-5   帧序号
        ,0x00,0x6E    // 6-7   帧长
        ,DEVICE_ID    // 8-11  设备ID
        ,TARGET_ID    // 12-15 目标ID
        ,0x00,0x72    // 16-17 命令字
        ,0x00,0x02    // 18-19 状态字：00-失败，01-所有通道，02-单通道
        ,0x00,0x00    // 20-21 通道
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor0 22-27
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor1 28-33
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor2 34-39
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor3 40-45
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor4 46-51
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor5 52-57
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor6 58-63
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor7 64-69
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor8 70-75
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor9 76-81
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor10 82-87
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor11 88-93
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor12 94-99
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor13 100-105
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor14 106-111
        ,0x00,0x00,0x00,0x00,0x00,0x00 // sensor15 112-117
    };

    // 解析操作类型 (18-19位)
    uint8_t operation = (G_USART1_RX_Buffer[18] << 8) | G_USART1_RX_Buffer[19];

    // 解析通道号 (20-21位)
    uint8_t channel = (G_USART1_RX_Buffer[20] << 8) | G_USART1_RX_Buffer[21];

    // 解析数值 (22-27位)
    uint32_t int_part = (G_USART1_RX_Buffer[22] << 16) | (G_USART1_RX_Buffer[23] << 8) | G_USART1_RX_Buffer[24];
    uint32_t dec_part = (G_USART1_RX_Buffer[25] << 16) | (G_USART1_RX_Buffer[26] << 8) | G_USART1_RX_Buffer[27];

    float value = (float)int_part + (float)dec_part / 1000000.0f;

    if(int_part > 1000000 || dec_part > 1000000)
    {
        Reply_72[19] = 0x00; // 表示失败
        HAL_UART_Transmit(&UartHandle, (uint8_t *)Reply_72, Reply_72_LENGTH, 20);
        while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET);
        return;
    }

    switch(operation)
    {
        case 0x0000: // 读取单个通道系数
            Reply_72[7] = 0x14; // 修改帧长度

            // 状态位
            Reply_72[18] = 0x00;
            Reply_72[19] = 0x02; // 0x02表示读取单个通道系数

            // 通道
            Reply_72[20] = G_USART1_RX_Buffer[20];
            Reply_72[21] = G_USART1_RX_Buffer[21];

            // 先读取flash
            Flash_Read(FLASH_COE_ADDR, sizeof(float) * SENSOR_NUM, (uint32_t *)coe_buffer);
            HAL_Delay(100);

            // 将对应通道的系数转成6字节数据
            process_value(coe_buffer[channel], Reply_72, 22);
            
            // 发送回复
            HAL_UART_Transmit(&UartHandle, (uint8_t *)Reply_72, Reply_72_LENGTH, 20);
            while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET);
            break;
        case 0x0001: // 读取所有通道系数
            // 状态位
            Reply_72[18] = 0x00;
            Reply_72[19] = 0x01; // 0x01表示所有通道

            // 先读取flash
            Flash_Read(FLASH_COE_ADDR, sizeof(float) * SENSOR_NUM, (uint32_t *)coe_buffer);

            // 处理采集到的16个电容值
            for(int i = 0; i < SENSOR_NUM; i++)
            {
                process_value(coe_buffer[i], Reply_72, 22 + i * 6);
            }
            
            // 发送回复
            HAL_UART_Transmit(&UartHandle, (uint8_t *)Reply_72, sizeof(Reply_72), 20);
            while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET);
            break;
        case 0x0002: // 设置单个通道系数
            // 先读取flash
            Flash_Read(FLASH_COE_ADDR, sizeof(float) * SENSOR_NUM, (uint32_t *)coe_buffer);
            
            // 修改帧长度
            Reply_72[7] = 0x14; 
            // 状态位
            Reply_72[18] = 0x00;
            Reply_72[19] = 0x02; // 0x02表示单个通道
            // 通道号
            Reply_72[20] = G_USART1_RX_Buffer[20];
            Reply_72[21] = G_USART1_RX_Buffer[21];
            
            // 设置单个通道系数
            coe_buffer[channel] = value;

            // 将对应通道的系数转成6字节数据
            process_value(coe_buffer[channel], Reply_72, 22);

            // 保存到flash
            Flash_Write(FLASH_COE_ADDR, sizeof(float) * SENSOR_NUM, (uint32_t *)coe_buffer);
            HAL_Delay(100);

            // 发送回复
            HAL_UART_Transmit(&UartHandle, (uint8_t *)Reply_72, Reply_72_LENGTH, 20);
            while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET);
            break;
    }
}


/**
 * @brief 处理99命令
 * 设置采集时间间隔
 */
void APP_Command_99(void)
{
    UART_RX_CMD = 0;

    // 解析receive time (20-21位)
    timer_coe = (G_USART1_RX_Buffer[20] << 8) | G_USART1_RX_Buffer[21];
    // printf("timer_coe: %d\r\n", timer_coe);

    Flash_Write_Status(FLASH_TIMER_ADDR, timer_coe); // 保存到flash
    HAL_Delay(100);

    Generic_Reply[7] = 0x0E;  // 设置帧长 14+8
    Generic_Reply[17] = 0x9A; // 设置回复
    Generic_Reply[20] = G_USART1_RX_Buffer[22];
    Generic_Reply[21] = G_USART1_RX_Buffer[23];

    // 回复指令
    HAL_UART_Transmit(&UartHandle, (uint8_t *)Generic_Reply, UART_RX_REPLY_20 + 2, 20); // 串口发送数据
    while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成    
}


/**
 * @brief 处理AA命令---校准寄生电容
 * 
 */
void APP_Command_AA(void)
{
    UART_RX_CMD = 0;

    Generic_Reply[7] = 0x0E;  // 设置帧长 14+8
    Generic_Reply[17] = 0x9A; // 设置回复
    Generic_Reply[20] = G_USART1_RX_Buffer[22];
    Generic_Reply[21] = G_USART1_RX_Buffer[23];

    // 回复指令
    HAL_UART_Transmit(&UartHandle, (uint8_t *)Generic_Reply, UART_RX_REPLY_20 + 2, 20); // 串口发送数据
    while (__HAL_UART_GET_FLAG(&UartHandle, UART_FLAG_TC) == RESET); // 等待发送完成
}



