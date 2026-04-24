#include "user_define.h"
#include "usart.h"

UART_HandleTypeDef UartHandle; // 串口句柄

volatile uint16_t G_setFlag = 0x00;    // 状态标识
volatile uint8_t G_USART1_RX_Flag = 0; // 空闲中断标记
uint8_t G_USART1_RX_Buffer[64] = {0};  // 串口1接收缓冲区
uint32_t G_USART1_RX_Count = 0;        // 串口1接收计数

/**
 * @brief USART1 GPIO初始化
 *
 */
void USART1_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE(); // 开启GPIOA模块时钟

    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;     // PA3|PA4
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;            // 推免复用
    GPIO_InitStruct.Pull = GPIO_PULLUP;                // 上拉模式
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH; // 超高速模式
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;       // 连接到USART1

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct); // 根据GPIO结构体初始化引脚
}

/**
 * @brief USART1初始化
 *
 * @param baudrate 波特率
 */
void USART1_Init(uint32_t baudrate)
{
    USART1_GPIO_Init(); // USART1 GPIO初始化

    /*使能时钟*/
    __HAL_RCC_USART1_CLK_ENABLE(); // 开启USART1模块时钟

    /*配置USART1参数*/
    UartHandle.Instance = USART1;                    // 串口基地址为USART1
    UartHandle.Init.BaudRate = baudrate;             // 波特率设置
    UartHandle.Init.WordLength = UART_WORDLENGTH_8B; // 数据位
    UartHandle.Init.StopBits = UART_STOPBITS_1;      // 停止位
    UartHandle.Init.Parity = UART_PARITY_NONE;       // 无校验
    UartHandle.Init.Mode = UART_MODE_TX_RX;          // 工作模式选择发送与接收
    UartHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE; // 无硬件控制流

    HAL_UART_Init(&UartHandle);                      // 根据结构体配置初始化USART1
    __HAL_UART_ENABLE_IT(&UartHandle, UART_IT_IDLE); // 空闲中断使能

    /********************************************************************/
    // 由于HAL的机制 比较难用也难理解 所以这里采用操作寄存器的方式
    USART1->CR1 |= (1 << 2); // 使能 接收
    USART1->CR1 |= (1 << 3); // 使能 发送
    USART1->CR1 |= (1 << 4); // 使能 空闲中断
    USART1->CR1 |= (1 << 5); // 使能 接收中断
    USART1->DR &= 0x00;      // 清空数据寄存器

    /* 使能NVIC */
    HAL_NVIC_SetPriority(USART1_IRQn, 1, 1);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

/**
 * @brief 串口中断函数
 *
 */
void USART1_IRQHandler(void)
{
    if ((USART1->SR & (1 << 5)) != 0) // 接收存入数组中
    {
        if (G_USART1_RX_Count < 256) // 防止溢出
        {
            G_USART1_RX_Buffer[G_USART1_RX_Count] = (uint8_t)USART1->DR; // 读取数据
            G_USART1_RX_Count++;
            /* 清除标记位 */
            USART1->SR &= ~(1 << 5);
        }
    }
    if ((USART1->SR & (1 << 4)) != 0) // 产生空闲中断
    {
        G_USART1_RX_Flag = 1; // 接收完成标志置1
        /*清除中断标志位*/
        USART1->SR;
        USART1->DR;
    }
}

/**
 * @brief 串口发送数据函数（1字节）
 *
 * @param dat 数据
 */
void USART1_Send_Byte(uint8_t dat)
{
    while ((USART1->SR & (1 << 6)) == 0)
        ;             // 等待上一个数据发送完成
    USART1->DR = dat; // 将数据写入DR寄存器
}

/**
 * @brief 串口发送字符串
 *
 * @param str 字符串
 */
void USART1_Send_String(char *str)
{
    while (*str) // 当字符串不为空
    {
        USART1_Send_Byte(*str); // 发送当前字符
        str++;                  // 移动到下一个字符
    }
}

/**
 *
 * @param ch 字符
 * @param f 文件指针
 * @return int 字符
 */
void USART1_Send_Hex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";

    USART1_Send_Byte(hex[value >> 4]);
    USART1_Send_Byte(hex[value & 0x0F]);
}

void USART1_Send_Bytes(uint8_t *data, uint16_t size)
{
    for (uint16_t i = 0; i < size; i++)
    {
        USART1_Send_Byte(data[i]);
    }
}

/**
 * @brief 串口错误处理函数
 *
 * @param huart 串口句柄
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    (void)huart;
}

__IO static uint8_t tx_buf[64] = {0};
void uart_send_data(float *data)
{

    // 仅仅复制4个点的数据
    memcpy((uint8_t *)tx_buf, (uint8_t *)data, sizeof(float) * 4);

    tx_buf[16] = 0x0d; // \r
    tx_buf[17] = 0x0a; // \n

    // 发送数据到U0 DMA
    USART1_Send_Bytes((uint8_t *)tx_buf, 18);
}

void uart_send_multi_data(void)
{
    float result = result_data[0];

    // 第一步, 复制通道1最终数据
    memcpy((uint8_t *)tx_buf, (uint8_t *)&result, sizeof(float));

    // 第二步, 复制通道1基线值
    memcpy((uint8_t *)tx_buf + 4, (uint8_t *)&g.base_line[0], sizeof(float));

    // 第三步, 复制通道1噪声值
    float diff = g.base_noise[0] * K_TH_F;
    memcpy((uint8_t *)tx_buf + 8, (uint8_t *)&diff, sizeof(float));

    // 第四步, 复制通道1采样值
    memcpy((uint8_t *)tx_buf + 12, (uint8_t *)&c_real_time_value[0], sizeof(float));

    // float if_over_th = (float)g.over_th[0];
    // memcpy(tx_buf + 16, (uint8_t *)&if_over_th, sizeof(float));
    float status = (float)c0_wave_detector.state;
    memcpy((uint8_t *)tx_buf + 16, (uint8_t *)&status, sizeof(float));

    tx_buf[20] = 0x0d; // \r
    tx_buf[21] = 0x0a; // \n

    // 发送数据到U0 DMA
    USART1_Send_Bytes((uint8_t *)tx_buf, 22);
}

void uart_send_float_data(float data)
{
    memcpy((uint8_t *)tx_buf, (uint8_t *)&data, sizeof(float));
    tx_buf[4] = 0x0d; // \r
    tx_buf[5] = 0x0a; // \n
    USART1_Send_Bytes((uint8_t *)tx_buf, 6);
}

void uart_send_gsensor_axes(int16_t x, int16_t y, int16_t z)
{
    uint8_t frame[11];

    frame[0] = GSENSOR_UART_HEADER_0;
    frame[1] = GSENSOR_UART_HEADER_1;
    frame[2] = GSENSOR_UART_TYPE_RAW;
    frame[3] = (uint8_t)(x & 0xFF);
    frame[4] = (uint8_t)((x >> 8) & 0xFF);
    frame[5] = (uint8_t)(y & 0xFF);
    frame[6] = (uint8_t)((y >> 8) & 0xFF);
    frame[7] = (uint8_t)(z & 0xFF);
    frame[8] = (uint8_t)((z >> 8) & 0xFF);
    frame[9] = 0x0D;
    frame[10] = 0x0A;

    USART1_Send_Bytes(frame, sizeof(frame));
}

void uart_send_gsensor_event(uint8_t success, uint8_t fail_mask, int16_t peak_any, uint16_t energy, uint8_t active_samples, uint8_t rebound_detected)
{
    uint8_t frame[13];

    frame[0] = GSENSOR_UART_HEADER_0;
    frame[1] = GSENSOR_UART_HEADER_1;
    frame[2] = GSENSOR_UART_TYPE_EVENT;
    frame[3] = success;
    frame[4] = fail_mask;
    frame[5] = (uint8_t)(peak_any & 0xFF);
    frame[6] = (uint8_t)((peak_any >> 8) & 0xFF);
    frame[7] = (uint8_t)(energy & 0xFF);
    frame[8] = (uint8_t)((energy >> 8) & 0xFF);
    frame[9] = active_samples;
    frame[10] = rebound_detected;
    frame[11] = 0x0D;
    frame[12] = 0x0A;

    USART1_Send_Bytes(frame, sizeof(frame));
}

int fputc(int ch, FILE *f)
{
    USART1_Send_Byte(ch);
    return ch;
}

