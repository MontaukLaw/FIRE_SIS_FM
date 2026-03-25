#include "iwdg.h"

IWDG_HandleTypeDef IwdgHandle; // 看门狗句柄

void IWDG_Init(void)
{
    // 看门狗初始化
    IwdgHandle.Instance = IWDG;                     // 独立看门狗的基地址
    IwdgHandle.Init.Prescaler = IWDG_PRESCALER_256; // 使用最大分频系数256
    IwdgHandle.Init.Reload = 4095;                  // 使用最大重载值4095
    if (HAL_IWDG_Init(&IwdgHandle) != HAL_OK)       // 初始化看门狗
    {
        printf("\r\n\r\n\r\nWatchdog init failed!\r\n\r\n\r\n");
        while (1); // 进入死循环
    }
}


void feed_dog(void)
{
    if (HAL_IWDG_Refresh(&IwdgHandle) != HAL_OK)
    {
        printf("Watchdog refresh failed!\r\n");
        while (1); // 如果刷新失败，进入死循环
    }
}







