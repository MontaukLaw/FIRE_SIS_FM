#include "main.h"

#define SPI_CS_LOW() HAL_GPIO_WritePin(SPI_NSS_PORT, SPI_NSS_PIN, GPIO_PIN_RESET)
#define SPI_CS_HIGH() HAL_GPIO_WritePin(SPI_NSS_PORT, SPI_NSS_PIN, GPIO_PIN_SET)

SPI_HandleTypeDef Spi1Handle;

// PA1 -> SPI1_MISO
// PA0 -> SPI1_MOSI
// PB0 -> SPI1_SCK
// PB5 -> SPI1_NSS
void mh1612s_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE(); /* GPIOB clock enable */
    __HAL_RCC_GPIOA_CLK_ENABLE(); /* GPIOA clock enable */
    __HAL_RCC_SPI1_CLK_ENABLE();  /* SPI1 clock enable */

    /*SCK*/
    GPIO_InitStruct.Pin = SPI_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    // 时钟极性是 SPI_CPOL_Low（空闲低电平）
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

    // PB0 复用为 SPI1_SCK
    GPIO_InitStruct.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(SPI_SCK_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SPI_MISO_PIN || SPI_MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF0_SPI1; // confirmed
    HAL_GPIO_Init(SPI_MISO_PORT, &GPIO_InitStruct);

    // NFC PWDN 配置为普通 GPIO 输出
    GPIO_InitStruct.Pin = NFC_PWDN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(NFC_PWDN_PORT, &GPIO_InitStruct);

    /* SPI NSS 普通gpio*/
    GPIO_InitStruct.Pin = SPI_NSS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; // NSS配置为普通GPIO输出
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SPI_NSS_PORT, &GPIO_InitStruct);

    // NFC INT 配置为EXIT输入, 上升沿触发
    GPIO_InitStruct.Pin = NFC_INT_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;   // 上升沿触发
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;         // 下拉
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; /* 高速 */
    HAL_GPIO_Init(NFC_INT_PORT, &GPIO_InitStruct);

    /* Interrupt Configuration */
    HAL_NVIC_SetPriority(SPI1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(SPI1_IRQn);

    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0); // NFC INT优先级更高
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
}

void spi1_init(void)
{
    /* De-initialize SPI configuration */
    Spi1Handle.Instance = SPI1;                                    /* SPI1 */
    Spi1Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256; /* 256 frequency division */
    Spi1Handle.Init.Direction = SPI_DIRECTION_2LINES;              /* full duplex */
    Spi1Handle.Init.CLKPolarity = SPI_POLARITY_LOW;                /* 时钟信号空闲低电平 */
    Spi1Handle.Init.CLKPhase = SPI_PHASE_1EDGE;                    /* 数据采样从第一个时钟沿开始 */
    Spi1Handle.Init.DataSize = SPI_DATASIZE_8BIT;                  /* SPI数据长度为8位 */
    Spi1Handle.Init.FirstBit = SPI_FIRSTBIT_MSB;                   /* 先发送MSB */
    Spi1Handle.Init.NSS = SPI_NSS_SOFT;                            /* NSS软件模式 */
    Spi1Handle.Init.Mode = SPI_MODE_MASTER;                        /* 配置为主机 */

    if (HAL_SPI_DeInit(&Spi1Handle) != HAL_OK)
    {
        APP_ErrorHandler();
    }

    /* SPI initialization */
    if (HAL_SPI_Init(&Spi1Handle) != HAL_OK)
    {
        APP_ErrorHandler();
    }
}

#if 0
HAL_StatusTypeDef nfc_write_reg(uint8_t addr, uint8_t value)
{
    uint8_t data_buf[2], rx_buf[2];
    int status, rx_len;
    data_buf[0] = ((uint8_t)(addr << 1UL)) & 0x7E;
    data_buf[1] = value;

    // 拉低NSS引脚，开始SPI通信
    SPI_CS_LOW();
    HAL_StatusTypeDef rtn = HAL_SPI_Transmit(&Spi1Handle, data_buf, 2, HAL_MAX_DELAY);
    if (rtn != HAL_OK)
    {
        printf("SPI transmit error: %d\r\n", rtn);
    }
    SPI_CS_HIGH();
    return rtn;
}

HAL_StatusTypeDef nfc_read_reg(uint8_t addr, uint8_t *value)
{
    uint8_t tx_buf[2], rx_buf[2];
    int status, rx_len;
    data_buf[0] = ((uint8_t)(addr << 1UL)) | 0x80;
    tx_buf[1] = 0xFF; // 占位字节

    // 拉低NSS引脚，开始SPI通信
    SPI_CS_LOW();
    HAL_StatusTypeDef rtn = HAL_SPI_TransmitReceive(&Spi1Handle, tx_buf, rx_buf, 2, HAL_MAX_DELAY);
    if (rtn != HAL_OK)
    {
        printf("SPI transmit error: %d\r\n", rtn);
        SPI_CS_HIGH();
        return rtn;
    }
    SPI_CS_HIGH();

    *value = rx_buf[1]; // 第二个字节是寄存器值
    return HAL_OK;
}
#endif

HAL_StatusTypeDef nfc_write_reg(uint8_t addr, uint8_t value)
{
    uint8_t tx_buf[2];

    tx_buf[0] = (uint8_t)((addr & 0x3F) << 1); // 写: bit7=0, bit0=0
    tx_buf[1] = value;

    SPI_CS_LOW();
    HAL_StatusTypeDef rtn = HAL_SPI_Transmit(&Spi1Handle, tx_buf, 2, HAL_MAX_DELAY);
    SPI_CS_HIGH();

    if (rtn != HAL_OK)
    {
        printf("SPI write error: %d\r\n", rtn);
    }

    return rtn;
}

HAL_StatusTypeDef nfc_read_reg(uint8_t addr, uint8_t *value)
{
    uint8_t tx_buf[2], rx_buf[2];

    if (value == NULL)
    {
        return HAL_ERROR;
    }

    tx_buf[0] = (uint8_t)(0x80 | ((addr & 0x3F) << 1)); // 读: bit7=1, bit0=0
    tx_buf[1] = 0x00;                                   // dummy byte

    SPI_CS_LOW();
    HAL_StatusTypeDef rtn = HAL_SPI_TransmitReceive(&Spi1Handle, tx_buf, rx_buf, 2, HAL_MAX_DELAY);
    SPI_CS_HIGH();

    if (rtn != HAL_OK)
    {
        printf("SPI read error: %d\r\n", rtn);
        return rtn;
    }

    *value = rx_buf[1]; // 第二个字节是返回的寄存器值
    return HAL_OK;
}

/* 读卡器复位 */
static inline void hal_nfc_pcd_reset(void)
{
    nfc_write_reg(REG_COMMAND, PCD_RESETPHASE);
}

void mh1612s_init(void)
{
    mh1612s_gpio_init();
    spi1_init();

    // 拉低NFC_PWDN引脚，确保MH1612S上电
    HAL_GPIO_WritePin(NFC_PWDN_PORT, NFC_PWDN_PIN, GPIO_PIN_RESET);

    printf("MH1612S reset\r\n");
    
    hal_nfc_pcd_reset();

}

void nfc_set_reg(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel)
{
    // diable mcu irq
    __disable_irq();

    if (page_sel)
    {
        switch (page)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            nfc_write_reg(REG_PAGESEL, 0);
            break;

        case 4:
            nfc_write_reg(REG_PAGESEL, 0x5e);
            break;

        case 5:
            nfc_write_reg(REG_PAGESEL, 0xae);
            break;

        case 6:
            nfc_write_reg(REG_PAGESEL, 0x5a);
            break;

        case 7:
            nfc_write_reg(REG_PAGESEL, 0xa5);
            break;

        case 8:
            nfc_write_reg(REG_PAGESEL, 0x55);
            break;

        case 9:
            nfc_write_reg(REG_PAGESEL, 0x95);
            break;

        case 10:
            nfc_write_reg(REG_PAGESEL, 0x3e);
            break;

        default:
            __enable_irq();
            return;
        }
    }

    nfc_write_reg(addr, value);
    // enable mcu irq
    __enable_irq();
}

uint8_t nfc_get_reg(uint8_t page, uint8_t addr, uint8_t page_sel)
{

    __disable_irq();
    uint8_t reg;
    if (page_sel) // 需要重新选择页码
    {
        switch (page)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            nfc_write_reg(REG_PAGESEL, 0);
            break;

        case 4:
            nfc_write_reg(REG_PAGESEL, 0x5e);
            break;

        case 5:
            nfc_write_reg(REG_PAGESEL, 0xae);
            break;

        case 6:
            nfc_write_reg(REG_PAGESEL, 0x5a);
            break;

        case 7:
            nfc_write_reg(REG_PAGESEL, 0xa5);
            break;

        case 8:
            nfc_write_reg(REG_PAGESEL, 0x55);
            break;

        case 9:
            nfc_write_reg(REG_PAGESEL, 0x95);
            break;

        case 10:
            nfc_write_reg(REG_PAGESEL, 0x3e);
            break;

        default:
            __enable_irq();
            return 0xFF;
        }
    }

    // 读功能寄存器
    nfc_read_reg(addr, &reg);

    __enable_irq();
    return reg;
}

void nfc_set_bit(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel)
{
    nfc_set_reg(page, addr, nfc_get_reg(page, addr, page_sel) | value, 1);
}

void nfc_clear_bit(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel)
{
    nfc_set_reg(page, addr, nfc_get_reg(page, addr, page_sel) & ~value, 1);
}

uint8_t nfc_check_bit(uint8_t page, uint8_t addr, uint8_t value, uint8_t page_sel)
{
    return (nfc_get_reg(page, addr, page_sel) & value) ? 1 : 0;
}

void nfc_clear_reg_bit(uint8_t addr, uint8_t mask)
{
    uint8_t tmp;

    tmp = nfc_get_reg(0, addr, 0);
    nfc_set_reg(0, addr, tmp & ~mask, 0);
}

void nfc_set_reg_bit(uint8_t addr, uint8_t mask)
{
    uint8_t tmp;

    tmp = nfc_get_reg(0, addr, 0);
    nfc_set_reg(0, addr, tmp | mask, 0);
}
