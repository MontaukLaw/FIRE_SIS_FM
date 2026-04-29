#include "main.h"

#define SLEEP_DEBUG 0

#if SLEEP_DEBUG
#define SLEEP_LOG(...) printf(__VA_ARGS__)
#else
#define SLEEP_LOG(...)
#endif

static void I2C_Bus_Recovery(void)
{
    uint8_t i;
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(1);

    for (i = 0; i < 9; i++)
    {
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_SET)
        {
            break;
        }

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
        HAL_Delay(1);
    }

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(1);
}

#if SLEEP_DEBUG
static void I2C_Log_DeviceReady(uint8_t addr)
{
    HAL_StatusTypeDef status;

    status = HAL_I2C_IsDeviceReady(&I2cHandle, (uint16_t)addr << 1, 3, 50);
    SLEEP_LOG("i2c ready 0x%02x: %d err:0x%08lx\r\n", addr, status, (unsigned long)HAL_I2C_GetError(&I2cHandle));
}
#endif

static void RM_VDD_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = RM_VDD_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RM_VDD_PORT, &GPIO_InitStruct);
}

static void RM_VDD_Log(const char *tag)
{
#if SLEEP_DEBUG
    SLEEP_LOG("rm vdd %s idr:%d odr:%d MODER:0x%08lx OTYPER:0x%08lx PUPDR:0x%08lx\r\n",
              tag,
              HAL_GPIO_ReadPin(RM_VDD_PORT, RM_VDD_PIN),
              ((RM_VDD_PORT->ODR & RM_VDD_PIN) != 0U),
              (unsigned long)RM_VDD_PORT->MODER,
              (unsigned long)RM_VDD_PORT->OTYPER,
              (unsigned long)RM_VDD_PORT->PUPDR);
#else
    (void)tag;
#endif
}

static void RM_RST_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = RM_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RM_RST_PORT, &GPIO_InitStruct);
}

static void RM_RST_Log(const char *tag)
{
#if SLEEP_DEBUG
    SLEEP_LOG("rm rst %s idr:%d odr:%d\r\n",
              tag,
              HAL_GPIO_ReadPin(RM_RST_PORT, RM_RST_PIN),
              ((RM_RST_PORT->ODR & RM_RST_PIN) != 0U));
#else
    (void)tag;
#endif
}

void sleep_test_task(void)
{
    uint8_t i;
    for (i = 0; i < 5; i++)
    {
        printf("Test task running: %d\r\n", i);
        HAL_Delay(1000);
    }

    SLEEP_LOG("Simulating wakeup event\r\n");

    // 进 STOP 前，先反初始化 I2C
    HAL_I2C_DeInit(&I2cHandle);
    __HAL_I2C_RESET_HANDLE_STATE(&I2cHandle);

    stop_rm1002();

    // i2c引脚设置为高阻态
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // B3 B4
    GPIO_InitStruct.Pin = GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    RM_RST_GPIO_Init();
    HAL_GPIO_WritePin(RM_RST_PORT, RM_RST_PIN, GPIO_PIN_RESET);
    RM_RST_Log("hold low");

    RM_VDD_GPIO_Init();
    RM_VDD_Log("before off");
    HAL_GPIO_WritePin(RM_VDD_PORT, RM_VDD_PIN, GPIO_PIN_RESET);
    RM_VDD_Log("after off");
    HAL_Delay(100);

    __HAL_GPIO_EXTI_CLEAR_IT(G_SENSOR_INT_PIN);
    __HAL_GPIO_EXTI_CLEAR_IT(NFC_INT_PIN);

    // 进入睡眠模式前，先关闭 SysTick 中断，防止它唤醒 MCU
    HAL_SuspendTick();

    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    // 程序会在这里暂停.

    System_Clock_Config_HSI_24Mhz(); // 系统时钟初始化
    HAL_ResumeTick();

    SLEEP_LOG("Woke up from sleep\r\n");

    // 拉高1002B电源
    RM_VDD_GPIO_Init();
    HAL_GPIO_WritePin(RM_VDD_PORT, RM_VDD_PIN, GPIO_PIN_SET);
    RM_VDD_Log("after set high");
    HAL_Delay(300);
    RM_VDD_Log("after 300ms");
    HAL_GPIO_WritePin(RM_RST_PORT, RM_RST_PIN, GPIO_PIN_SET);
    RM_RST_Log("release high");
    HAL_Delay(50);

    I2C_Bus_Recovery();
    HAL_Delay(50);

    SLEEP_LOG("i2c pins before init scl:%d sda:%d\r\n",
              HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3),
              HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4));

    I2C_Init();
    SLEEP_LOG("i2c init done state:%d err:0x%08lx\r\n",
              HAL_I2C_GetState(&I2cHandle),
              (unsigned long)HAL_I2C_GetError(&I2cHandle));
#if SLEEP_DEBUG
    I2C_Log_DeviceReady(0x2b);
    I2C_Log_DeviceReady(0x28);
#endif

    // 重新初始化1002b
    RM_Init();
}

// 低功耗相关
void sleep_check_task(void)
{

    static uint32_t last_run_ts = 0;

    if (HAL_GetTick() - last_run_ts < 1000) // 每秒检查一次
    {
        return;
    }

    SLEEP_LOG("Checking sleep conditions\r\n");
    last_run_ts = HAL_GetTick();

    // 检查是否静止了5秒, 如果是则进入睡眠模式
    if (HAL_GetTick() - last_active_ts < SLEEP_CHECK_MAX)
    {
        // printf("Skip sleep\r\n");
        return;
    }

    // 清 EXTI 挂起
    __HAL_GPIO_EXTI_CLEAR_IT(G_SENSOR_INT_PIN);

    // 再给一个很短的安静确认窗口
    HAL_Delay(20);

    // power_down_gsensor();

    // deinit i2c
    HAL_I2C_DeInit(&I2cHandle);
    __HAL_I2C_RESET_HANDLE_STATE(&I2cHandle);

    I2C_GPIO_DeInit();

    HAL_GPIO_WritePin(RM_VDD_PORT, RM_VDD_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(RM_RST_PORT, RM_RST_PIN, GPIO_PIN_RESET);

    // gsensor下电
    // HAL_GPIO_WritePin(SC7A20_VDD_PORT, SC7A20_VDD_PIN, GPIO_PIN_RESET);

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = RM_VDD_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG; /* Hold the external power switch off. */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RM_VDD_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = RM_RST_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG; /* Avoid a floating reset/input path. */
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RM_RST_PORT, &GPIO_InitStruct);

    __HAL_RCC_I2C_CLK_DISABLE();

    // HAL_GPIO_WritePin(SC7A20_VDD_PORT, SC7A20_VDD_PIN, GPIO_PIN_RESET);

    SLEEP_LOG("Entering sleep mode\r\n");

    // 进入睡眠模式前，先关闭 SysTick 中断，防止它唤醒 MCU
    HAL_SuspendTick();

    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    System_Clock_Config_HSI_24Mhz(); // 系统时钟初始化
    HAL_ResumeTick();

    exti_triggered_ts = HAL_GetTick(); // 重置时间戳，防止立即再次进入睡眠

    SLEEP_LOG("Woke up from sleep\r\n");

    // HAL_GPIO_WritePin(RM_VDD_PORT, RM_VDD_PIN, GPIO_PIN_SET);
    // HAL_GPIO_WritePin(RM_RST_PORT, RM_RST_PIN, GPIO_PIN_SET);

    HAL_Delay(300);

    rm_init();
}
