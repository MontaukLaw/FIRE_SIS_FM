#include "main.h"
#include "tap_algo.h"

#define MAIN_DEBUG DEBUG_LOG_ENABLE

#if MAIN_DEBUG
#define MAIN_LOG(...) printf(__VA_ARGS__)
#else
#define MAIN_LOG(...)
#endif

uint32_t last_active_ts = 0;
uint8_t shaked = 0;
static uint32_t gsensor_uart_stream_until = 0;

static void key_init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // A5
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL; // GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin = KEY_INT_PIN;
    HAL_GPIO_Init(KEY_INT_PORT, &GPIO_InitStruct);
}

static void exti_config(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // A7
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL; // GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pin = G_SENSOR_INT_PIN;
    HAL_GPIO_Init(G_SENSOR_INT_PORT, &GPIO_InitStruct);

    // A7
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
}

int main(void)
{
    uint32_t last_sample_ts = 0;
    uint32_t last_debug_ts = 0;

    app_config();

    exti_config();

    // key_init();

    HAL_Delay(200);

    wave_detector_init(&c0_wave_detector);

    // power_down_gsensor();
    // init_gsensor_for_lp_int();

    // 模拟i2c初始化gsensor
    init_gsenor_sim();

    // init_gsensor_for_lp_int_sim();
    // deinit_gsensor_gpio();
    // init_gsensor_for_interrupt();

    MAIN_LOG("Sys start\r\n");
    bool tap_detected = false;
    uint8_t counter = 0;
    uint16_t i;
    int16_t acc_data[3] = {0};

    while (1)
    {
        // float c0_diff;
        read_rmof_addres3();

        baseline_tracking((float *)c_real_time_value);

        // c0_diff = (c_real_time_value[0] > g.base_line[0]) ? (c_real_time_value[0] - g.base_line[0]) : 0.0f;
        wave_detector_process(&c0_wave_detector, diff_over_th[0], g.base_noise[0] * K_TH_F, g.base_noise[0] * K_TH_OFF_F);

        play_voice_base_wave_detector(c0_wave_detector);

        sleep_check_task();

        // uart_send_multi_data();
        // uart_send_wave_event();
    }

    while (0)
    {

        if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(KEY_INT_PORT, KEY_INT_PIN))
        {
            // MAIN_LOG("Key down\r\n");
            play_voice(1);
            HAL_Delay(200);
        }
        else
        {
            // MAIN_LOG("Key up\r\n");
        }
        // HAL_Delay(10);

        // init_gsenor_sim();
        // 第一步先确认所有外设工作正常. 先观察gsensor是否正常进中断
        // sc7a20_get_data_sim(acc_data);
        // MAIN_LOG("Acc: %6d %6d %6d\r\n", acc_data[0], acc_data[1], acc_data[2]);
        // printf("Loop... %lu\r\n", HAL_GetTick());
        // HAL_Delay(500);
    }

    while (0)
    {
        // printf("Loop... %lu\r\n", HAL_GetTick());
        // if (shaked)
        // {
        //     play_voice(1);
        //     shaked = 0;
        // }
        // sc7a20_get_data_sim(acc_data);
        // MAIN_LOG("Acc: %6d %6d %6d\r\n", acc_data[0], acc_data[1], acc_data[2]);
        // HAL_Delay(500);
    }

    while (0)
    {
        simple_shake_and_play_task();
        sleep_check_task();
    }

    while (0)
    {
        sleep_check_task();
    }

    while (1)
    {
        float c0_diff;

        read_rmof_addres3();

        baseline_tracking((float *)c_real_time_value);

        // c0_diff = (c_real_time_value[0] > g.base_line[0]) ? (c_real_time_value[0] - g.base_line[0]) : 0.0f;
        wave_detector_process(&c0_wave_detector, diff_over_th[0], g.base_noise[0] * K_TH_F, g.base_noise[0] * K_TH_OFF_F);

        // uart_send_multi_data();

        play_voice_base_wave_detector(c0_wave_detector);

        HAL_Delay(1);

        simple_shake_and_play_task();

        sleep_check_task();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == G_SENSOR_INT_PIN)
    {
        // last_active_ts = HAL_GetTick();
        // shaked = 1;
        MAIN_LOG("shaked\r\n");
    }
    else if (GPIO_Pin == NFC_INT_PIN)
    {
        // shaked = 1;
        // printf("KeyDown\r\n");
        MAIN_LOG("Key down\r\n");
    }
}
