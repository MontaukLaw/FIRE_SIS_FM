#include "sys.h"

void APP_Config(void)
{
    HAL_Init();                      // HAL库初始化
    System_Clock_Config_HSI_24Mhz(); // 系统时钟初始化

    APP_GPIOInit(); // GPIO初始化

    I2C_GPIO_Init(); // I2C GPIO初始化
    // IWDG_Init(); // 看门狗初始化

    APP_TimConfig();     // 定时器初始化
    USART1_Init(115200); // 初始化串口

    /* I2C initialization */
    I2C_Init();

    RM_Init();    // 锐盟初始化
    Flash_Init(); // Flash初始化
}

/**
 * @brief 锐盟初始化
 * PA5->RM0_RST
 * PB0->RM1_RST
 * PA0->RM2_RST
 * PA1->RM3_RST
 * 全部拉低
 * 然后依次拉高初始化
 */
void RM_Init(void)
{

    // 锐盟初始化
    rm1002_init(DEFAULE_I2C_ADDR_3);

    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, GPIO_PIN_SET); // RM2
    // RM1002B_X = DEFAULE_I2C_ADDR_2;                     // RM2
    // rm1002_init(RM1002B_X);

    // HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // RM1
    // RM1002B_X = DEFAULE_I2C_ADDR_1;                     // RM1
    // rm1002_init(RM1002B_X);

    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET); // RM0
    // RM1002B_X = DEFAULE_I2C_ADDR_0;                     // RM0
    // rm1002_init(RM1002B_X);
}

// 配置系统时钟 HSI 24Mhz
void System_Clock_Config_HSI_24Mhz(void)
{
    /* OSC 配置 */
    RCC_OscInitTypeDef RCC_OscInitStruct;

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;        // 配置类型 HSI
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;                          // 打开HSI
    RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;                          // HSI不分频
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_24MHz; // HSI输入 24Mhz

    HAL_RCC_OscConfig(&RCC_OscInitStruct); // 根据配置初始化OSC

    /* 系统时钟配置 */
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_PCLK1;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI; // 系统时钟源 ---->HSI 24Mhz
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;     // AHB总线 不分频 24MHZ
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;      // APB总线 AHB 2分频 12Mhz

    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0); // Flash 0等待周期
}

/**
 * @brief 写入数组初始化
 *
 */
void Array_Write_Init(void)
{
    for (int i = 0; i < 16; i++)
    {
        coe_buffer[i] = 1.0f;
    }
    timer_coe = 1;
}

float ema_float(float last, float now, uint16_t alpha_num, uint16_t alpha_den)
{
    return (last * (alpha_den - alpha_num) + now * alpha_num) / alpha_den;
}

float float_asb_diff(float a, float b)
{
    return (a > b) ? (a - b) : (b - a);
}

float low_filter_op(float pre_val, float new_val, float *data_ptr, float alpha)
{
    float val = pre_val * (1 - alpha) + new_val * alpha;
    *data_ptr = val;
    return val;
}

/*****************************************************************************
 * Name		: is_timeout
 * Function	: Determine whether the timeout that millisecond.
 * ---------------------------------------------------------------------------
 * Input Parameters:
 *			start_time	start time
 *			interval		time interval
 * Output Parameters:None
 * Return Value:
 *			TRUE		timeout
 *			FALSE		It is not timeout
 * ---------------------------------------------------------------------------
 * Description:
 *
 *****************************************************************************/
int is_timeout(tick start_time, tick interval)
{
    return (get_diff_tick(get_tick(), start_time) >= interval);
}

/*****************************************************************************
 * Name		: get_diff_tick
 * Function	: Get the time interval (unit is 1ms) for two tick.
 * ---------------------------------------------------------------------------
 * Input Parameters:
 *			cur_tick		lastest tick
 *			prior_tick		prior tick
 * Output Parameters:None
 * Return Value:
 *			Return the ticks of two tick difference.
 * ---------------------------------------------------------------------------
 * Description:
 *
 *****************************************************************************/
tick get_diff_tick(tick cur_tick, tick prior_tick)
{
    if (cur_tick < prior_tick)
    {
        return (cur_tick + (~prior_tick));
    }
    else
    {
        return (cur_tick - prior_tick);
    }
}
