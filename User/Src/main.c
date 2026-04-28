#include "main.h"
#include "tap_algo.h"

#define MAIN_DEBUG 0

#if MAIN_DEBUG
#define MAIN_LOG(...) printf(__VA_ARGS__)
#else
#define MAIN_LOG(...)
#endif

uint8_t shaked = 0;
static uint32_t gsensor_uart_stream_until = 0;

static void exti_config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Pin = NFC_INT_PIN;
    HAL_GPIO_Init(NFC_INT_PORT, &GPIO_InitStruct);

    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
}

int main(void)
{
    uint32_t last_sample_ts = 0;
    uint32_t last_debug_ts = 0;

    app_config();

    exti_config();

    HAL_Delay(200);

    wave_detector_init(&c0_wave_detector);

    MAIN_LOG("Sys start\r\n");
    bool tap_detected = false;
    uint8_t counter = 0;
    uint16_t i;

    while (1)
    {
        simple_shake_and_play_task();

        sleep_check_task();

    }

    while (1)
    {
        read_rmof_addres3();

        baseline_tracking((float *)c_real_time_value);

        wave_detector_process(&c0_wave_detector, result_data[0], g.base_line[0] * K_TH_F, g.base_line[0] * K_TH_OFF_F);

        // uart_send_multi_data();

        play_voice_base_wave_detector(c0_wave_detector);

        HAL_Delay(1);

        simple_shake_and_play_task();

        sleep_check_task();

        show_running();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == G_SENSOR_INT_PIN)
    {
    }
    else if (GPIO_Pin == NFC_INT_PIN)
    {

        shaked = 1;
        // printf("KeyDown\r\n");
        MAIN_LOG("Key down\r\n");
    }
}
