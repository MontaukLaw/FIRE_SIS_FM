#include "flash.h"

float cap_buffer[16] = {0};
float coe_buffer[16] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};
uint8_t MCU_Status = 0x00;

float cxxx0[4] = {13.806267, 12.406244, 12.293915, 12.196392};
float cxxx1[4] = {13.775604, 12.385233, 12.297818, 12.312315};
float cxxx2[4] = {13.550714, 12.053483, 12.082601, 11.999557};
float cxxx3[4] = {13.895423, 12.456123, 12.358680, 12.417278};

// 寄生电容擦除Flash
void CAP_FlashErase(uint32_t flash_addr, uint32_t buffer_size)
{
    uint32_t PAGEError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGEERASE;         /* 页擦除 */
    EraseInitStruct.PageAddress = flash_addr;                      /* 擦除起始地址 */
    EraseInitStruct.NbPages = 1;                                   // 为了正确计算擦除页数，需要向上取整
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK) /* 页擦除 */
    {
        printf("\r\nFlash Erase Error\r\n");
        APP_ErrorHandler();
    }
}

// 检查Flash是否为空
void CAP_FlashBlank(uint32_t flash_addr, uint32_t buffer_size)
{
    uint32_t addr = 0;

    while (addr < sizeof(buffer_size))
    {
        if (0xFFFFFFFF != HW32_REG(flash_addr + addr))
        {
            printf("\r\nFlash is not blank\r\n");
            APP_ErrorHandler();
        }
        addr += 4;
    }
}

// 将浮点数组存储到Flash
void Flash_Write(uint32_t flash_addr, uint32_t buffer_size, uint32_t *data_src)
{
    // 临时禁用看门狗
    IWDG_HandleTypeDef *pIwdg = &IwdgHandle;
    uint32_t reload = pIwdg->Init.Reload;
    pIwdg->Init.Reload = 0xFFF;
    HAL_IWDG_Init(pIwdg);

    uint8_t i = 0;
    uint32_t offset = 0;

    if (flash_addr == FLASH_CAP_ADDR)
    {
        // 将寄生电容数组都存入 cap_buffer 中
        for (i = 0; i < sizeof(cxxx0) / sizeof(float); i++, offset++)
        {
            cap_buffer[offset] = cxxx0[i];
        }
        for (i = 0; i < sizeof(cxxx1) / sizeof(float); i++, offset++)
        {
            cap_buffer[offset] = cxxx1[i];
        }
        for (i = 0; i < sizeof(cxxx2) / sizeof(float); i++, offset++)
        {
            cap_buffer[offset] = cxxx2[i];
        }
        for (i = 0; i < sizeof(cxxx3) / sizeof(float); i++, offset++)
        {
            cap_buffer[offset] = cxxx3[i];
        }
    }

    uint32_t flash_start_addr = flash_addr;
    uint32_t flash_end_addr = flash_addr + buffer_size;
    uint32_t *src = data_src;

    HAL_FLASH_Unlock();                      // 解锁Flash
    CAP_FlashErase(flash_addr, buffer_size); // 擦除Flash
    CAP_FlashBlank(flash_addr, buffer_size); // 检查Flash是否为空
    // 写入flash
    while (flash_start_addr < flash_end_addr)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_PAGE, flash_start_addr, src) == HAL_OK)
        {
            flash_start_addr += FLASH_PAGE_SIZE;
            src += FLASH_PAGE_SIZE / 4;
        }
    }
    HAL_FLASH_Lock(); // 上锁

    // 恢复看门狗设置
    pIwdg->Init.Reload = reload;
    HAL_IWDG_Init(pIwdg);
}

// 从Flash读取浮点数组
void Flash_Read(uint32_t flash_addr, uint32_t buffer_size, uint32_t *data_src)
{
    uint32_t addr = 0;
    uint32_t index = 0; // 数组索引计数器
    while (addr < buffer_size)
    {
        uint32_t value = HW32_REG(flash_addr + addr); // 从flash中读取4字节数据
        addr += 4;

        // 将 uint32_t 数据转换为 float，并存入数据缓冲区
        memcpy(&data_src[index], &value, sizeof(float));
        index++;
    }
}

// 写入单个状态值到Flash
void Flash_Write_Status(uint32_t flash_addr, uint8_t status)
{
    uint32_t status_buffer[16] = {0};    // 创建一个页大小的缓冲区
    status_buffer[0] = (uint32_t)status; // 将状态值放在缓冲区第一个位置

    HAL_FLASH_Unlock(); // 解锁Flash

    // 擦除状态值所在的页
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGEERASE;
    EraseInitStruct.PageAddress = flash_addr;
    EraseInitStruct.NbPages = 1; // 只需要一页

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
    {
        printf("\r\nStatus Flash Erase Error\r\n");
        APP_ErrorHandler();
    }

    // 写入状态值
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_PAGE, flash_addr, (uint32_t *)status_buffer) != HAL_OK)
    {
        printf("\r\nStatus Flash Write Error\r\n");
        APP_ErrorHandler();
    }

    HAL_FLASH_Lock(); // 上锁
}

// 从Flash读取状态值
uint8_t Flash_Read_Status(uint32_t flash_addr)
{
    uint32_t status_word = HW32_REG(flash_addr); // 读取32位数据
    return (uint8_t)status_word;                 // 转换回uint8_t
}

void Flash_Init(void)
{
    MCU_Status = Flash_Read_Status(FLASH_STATUS_ADDR);

    if (MCU_Status != MCU_STATUS_INIT) // 如果状态不为0，则进行初始化
    {
        printf("MCU_Flash Init\r\n");
        MCU_Status = MCU_STATUS_INIT;
        // 寄生电容
        // Array_Write_Init(); // 数组初始化
        Flash_Write(FLASH_CAP_ADDR, sizeof(cap_buffer), (uint32_t *)cap_buffer);
        HAL_Delay(100);
        // 归一化系数
        Flash_Write(FLASH_COE_ADDR, sizeof(coe_buffer), (uint32_t *)coe_buffer);
        HAL_Delay(100);
        // 状态
        Flash_Write_Status(FLASH_STATUS_ADDR, MCU_Status);
        HAL_Delay(100);
        // 定时器系数
        timer_coe = 1;
        Flash_Write_Status(FLASH_TIMER_ADDR, timer_coe);
        HAL_Delay(100);
    }
    Flash_Read(FLASH_CAP_ADDR, sizeof(cap_buffer), (uint32_t *)cap_buffer);
    Flash_Read(FLASH_COE_ADDR, sizeof(coe_buffer), (uint32_t *)coe_buffer);
    MCU_Status = Flash_Read_Status(FLASH_STATUS_ADDR);
    timer_coe = Flash_Read_Status(FLASH_TIMER_ADDR);
    // 修正寄生电容
    uint8_t i;
    for (i = 0; i < SENSOR_NUM; i++)
    {
        if (cap_buffer[i] < 10.0f)
        {
            cap_buffer[i] = 0.0f; // 最小值设为10pF
        }
        else
        {
            cap_buffer[i] = cap_buffer[i] - 10.0f; // 减去10pF基准值
        }
    }
}


/**
 * @brief 参数打印
 * 
 */
void Flash_Print(void)
{
    printf("cap_buffer: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\r\n", cap_buffer[0], cap_buffer[1], cap_buffer[2], cap_buffer[3], cap_buffer[4], cap_buffer[5], cap_buffer[6], cap_buffer[7], cap_buffer[8], cap_buffer[9], cap_buffer[10], cap_buffer[11], cap_buffer[12], cap_buffer[13], cap_buffer[14], cap_buffer[15]);
    printf("coe_buffer: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\r\n", coe_buffer[0], coe_buffer[1], coe_buffer[2], coe_buffer[3], coe_buffer[4], coe_buffer[5], coe_buffer[6], coe_buffer[7], coe_buffer[8], coe_buffer[9], coe_buffer[10], coe_buffer[11], coe_buffer[12], coe_buffer[13], coe_buffer[14], coe_buffer[15]);
    printf("MCU_Status: %d\r\n", MCU_Status);
    printf("timer_coe: %d\r\n", timer_coe);
}
