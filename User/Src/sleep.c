#include "main.h"

// 低功耗相关
void sleep_check_task(void)
{
    static uint32_t last_run_ts = 0;

    if (HAL_GetTick() - last_run_ts < 1000) // 每秒检查一次
    {
        return;
    }

    printf("Checking sleep conditions\r\n");
    last_run_ts = HAL_GetTick();

    // 检查是否静止了5秒, 如果是则进入睡眠模式
    if (HAL_GetTick() - exti_triggered_ts < SLEEP_CHECK_MAX)
    {
        // printf("Skip sleep\r\n");
        return;
    }

    // 清 EXTI 挂起
    __HAL_GPIO_EXTI_CLEAR_IT(G_SENSOR_INT_PIN);

    // 再给一个很短的安静确认窗口
    HAL_Delay(20);

    if (HAL_GPIO_ReadPin(G_SENSOR_INT_PORT, G_SENSOR_INT_PIN) == GPIO_PIN_SET)
    {
        printf("INT pin still active, skip sleep\r\n");
        return;
    }

    __HAL_GPIO_EXTI_CLEAR_IT(G_SENSOR_INT_PIN);

    printf("Entering sleep mode\r\n");

    // 进入睡眠模式前，先关闭 SysTick 中断，防止它唤醒 MCU
    HAL_SuspendTick();

    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    // HAL_PWR_EnterSTOPMode(PWR_DEEPLOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

    System_Clock_Config_HSI_24Mhz(); // 系统时钟初始化
    HAL_ResumeTick();

    exti_triggered_ts = HAL_GetTick(); // 重置时间戳，防止立即再次进入睡眠

    printf("Woke up from sleep\r\n");
}
