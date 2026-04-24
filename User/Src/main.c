#include "main.h"
#include "tap_algo.h"

uint32_t exti_triggered_ts = 0;
uint8_t shaked = 0;
static uint32_t gsensor_uart_stream_until = 0;

static void APP_ExtiConfig(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pin = G_SENSOR_INT_PIN;
    HAL_GPIO_Init(G_SENSOR_INT_PORT, &GPIO_InitStruct);

    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
}

TapCtx tap_ctx;
// Tap_Process() 运算时间大概是650us
// sc7a20_read_acc_raw() 运行时间大概是6个ms
int main(void)
{
    uint32_t last_sample_ts = 0;
    uint32_t last_debug_ts = 0;

    APP_Config();
    // delay 200ms
    HAL_Delay(200);

    sc7a20_init();

    Tap_Init(&tap_ctx);

    printf("Sys start\r\n");
    bool tap_detected = false;
    uint8_t counter = 0;
    uint16_t i;

    while (1)
    {
        uint32_t now = HAL_GetTick();

        /* 200Hz: 每 5ms 采样一次，只跑新的整数状态机。 */
        if ((now - last_sample_ts) < 5U)
            continue;

        sc7a20_reg_read_bytes_public(sc7a20_acc_data);
       
        tap_detected = Tap_Process(&tap_ctx, sc7a20_acc_data[0], sc7a20_acc_data[1], sc7a20_acc_data[2]);
        if (tap_detected)
        {
            play_voice(1);
        }
        
        // uart_send_gsensor_axes(sc7a20_acc_data[0], sc7a20_acc_data[1], sc7a20_acc_data[2]);

        last_sample_ts = now;
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == G_SENSOR_INT_PIN)
    {
    }
    else if (GPIO_Pin == NFC_INT_PIN)
    {
    }
}
