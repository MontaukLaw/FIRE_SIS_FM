#include "main.h"

float cxxx0[4] = {13.806267, 12.406244, 12.293915, 12.196392};
float cxxx1[4] = {13.775604, 12.385233, 12.297818, 12.312315};
float cxxx2[4] = {13.550714, 12.053483, 12.082601, 11.999557};
float cxxx3[4] = {13.895423, 12.456123, 12.358680, 12.417278};

uint32_t exti_triggered_ts = 0;
/**
 * @brief  External interrupt configuration function.
 * @param  None
 * @retval None
 */
static void APP_ExtiConfig(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE(); /* Enable GPIOA clock */

    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;   /* 上升沿中断 */
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;         /* 下拉 */
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; /* 高速 */
    GPIO_InitStruct.Pin = G_SENSOR_INT_PIN;
    HAL_GPIO_Init(G_SENSOR_INT_PORT, &GPIO_InitStruct);

    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);         /* Enable EXTI interrupt */
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0); /* Configure interrupt priority */
}

// for WFI test
int main(void)
{

    APP_Config();

    init_gsensor_for_interrupt();

    // 设置外部中断
    APP_ExtiConfig();

    mh1612s_init();

    // picc_init();

    while (1)
    {
        nfc_app();
        
        // sleep_check_task();
        // printf("Sys running\r\n");
    }
}

int main_(void)
{
    static uint16_t test_counter = 0;

    APP_Config();

    // printf("Sys start\r\n");

    WaveDetector_Init(&c0_wave_detector);

    sc7a20_init();
    float combin_acc = 0.0f;
    // float combin_acc_lp = 0.0f;

    while (1)
    {
        sc7a20_read_acc(sc7a20_acc_data, &combin_acc);
        play_voice_base_g(combin_acc);

        read_rmof_addres3();
        baseline_tracking((float *)c_real_time_value);

        WaveDetector_Process(&c0_wave_detector, result_data[0], g.base_noise[0] * K_TH_F, g.base_noise[0] * K_TH_OFF_F);
        uart_send_multi_data();
        play_voice_base_wave_detector(c0_wave_detector);

        HAL_Delay(1);
    }

    while (1)
    {
        get_all_parastic_cap();
        print_parastic_cap();
        HAL_Delay(5000);
    }

    while (1)
    {

        read_rmof_addres3();
        baseline_tracking((float *)c_real_time_value);

        // uart_send_data(c_real_time_value);
        // uart_send_data(result_data);

        WaveDetector_Process(&c0_wave_detector, result_data[0], g.base_line[0] * K_TH_F, g.base_line[0] * K_TH_OFF_F);
        uart_send_multi_data();

        play_voice_base_wave_detector(c0_wave_detector);

        // play_voice_base_result_data(result_data[0]);
        HAL_Delay(1);
    }

    while (0)
    {

        // 定期刷新看门狗
        // feed_dog();
        test_counter++;
        if (test_counter > 14)
        {
            test_counter = 1;
        }
        play_voice_no_repeat(8); // 播放语音1
        printf("playing voice %d\r\n", test_counter);
        // HAL_Delay(2000);
        // HAL_GPIO_TogglePin(LED_W_PORT, LED_W_PIN); // 切换LED状态

        // USART1_Send_String("Running\r\n");
        // get_all_parastic_cap();
        // print_parastic_cap();

        // read_rmof_addres3();
        // print_1002_data();
        HAL_Delay(3000);
    }
    return 0;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == G_SENSOR_INT_PIN)
    {
        printf("INT triggered\r\n");
        exti_triggered_ts = HAL_GetTick();
    }
    else if (GPIO_Pin == NFC_INT_PIN)
    {
        printf("NFC INT triggered\r\n");
    }
}
