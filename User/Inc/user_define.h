#ifndef _USER_DEEFINE_H_
#define _USER_DEEFINE_H_

#include "py32f0xx_hal.h"
#include <stdbool.h>
#include <math.h>

#define LED_W_PORT GPIOC
#define LED_W_PIN GPIO_PIN_1

#define VOICE_PORT GPIOB
#define VOICE_PIN GPIO_PIN_2

#define RM_VDD_PORT GPIOB
#define RM_VDD_PIN GPIO_PIN_7

#define SC7A20_VDD_PORT GPIOA
#define SC7A20_VDD_PIN GPIO_PIN_6

#define TOTAL_SENSOR_NUMBER 4

#define INIT_ZERO_TIMES 100

#define NOISE_MIN 1.0f

// 尽量灵敏
// K_TH_F = 3 ~ 5：比较敏感
// K_TH_F = 5 ~ 8：中等稳健
// K_TH_F = 8 ~ 12：更保守
// #define K_TH_F 5.0f
#define K_TH_F 5.0f
// 2.0~4.0f比较合适, 过小可能导致接触后无法离开, 过大可能导致接触后立即离开
#define K_TH_OFF_F 2.0f

#define S_NORM_ON 80      // 160     // 进入门槛
#define S_NORM_HOLD_NUM 3 // hold = 3/4 on
#define S_NORM_HOLD_DEN 4

#define S_NORM_OFF_NUM 1 // off  = 1/2 on
#define S_NORM_OFF_DEN 2

#define C_ON 2   // 进入：活跃通道数≥2
#define C_HOLD 1 // 维持：≥1
#define C_OFF 0  // 释放：=0

#define SON_NUM 3.0f
#define SON_DEN 2.0f

#define HOLD_NUM 3.0f
#define HOLD_DEN 3.0f

#define OFF_NUM 3.0f
#define OFF_DEN 4.0f

#define QUITE_TIMES 30
#define OVER_TH_TIMES 200

#define ACTIVATE_BASE_BETA_NUM 1
#define ACTIVATE_BASE_BETA_DEN 3

#define ACTIVATE_NOISE_BETA_NUM 1
#define ACTIVATE_NOISE_BETA_DEN 5

// 基线/噪声 EMA（β≈0.01，仅IDLE更新）
#define BASE_BETA_NUM 1
#define BASE_BETA_DEN 10

#define NOISE_IDEL_NUM 1
#define NOISE_IDEL_DEN 100

#define MAX_SENSORS TOTAL_SENSOR_NUMBER
#define MAX_CH_NMB MAX_SENSORS

typedef enum
{
    ST_IDLE = 0,
    ST_CONTACT_PENDING,
    ST_TRACKING,
    ST_ENDING
} TouchState;

struct g_baseline_tracking_t
{
    uint8_t initialized;
    uint8_t thresholds_ready;
    // 通道缓冲
    float base_line[MAX_SENSORS];
    float base_noise[MAX_SENSORS];

    float diff_f[MAX_SENSORS];
    float diff_norm_f[MAX_SENSORS]; // EMA 后的归一化值

    float val_over_threshold[MAX_SENSORS]; // 阈后强度（≥0）
    bool over_th[MAX_SENSORS];

    // 强度与质心
    float omiga_sig; // 幅度制总强度  Σ(sig)
    float sum_idx;   // 幅度制加权和  Σ(sig*i)
    float avg_th;    // 平均单通道门限
    uint16_t C_act;  // 活跃通道数

    float s_on, s_hold, s_off;                   // 幅度制
    uint32_t s_on_norm, s_hold_norm, s_off_norm; // 归一制
    uint16_t c_on, c_hold, c_off;

    uint32_t quiet_up_cnt[MAX_SENSORS];
    uint32_t over_th_cnt[MAX_SENSORS];
};

#define LEVEL_1 20
#define LEVEL_2 100
#define LEVEL_3 250

#define TH_MIN_ABS 3.0f

typedef enum
{
    WAVE_IDLE = 0,
    WAVE_ACTIVE,
    WAVE_REFRACTORY
} wave_state_t;

typedef struct
{
    wave_state_t state;

    // uint16_t th_on;
    // uint16_t th_off;

    uint8_t on_count;
    uint8_t off_count;

    uint8_t on_count_limit;  // 进入触发需连续点数
    uint8_t off_count_limit; // 退出触发需连续点数

    uint16_t width;
    uint16_t peak;
    uint32_t area;
    uint16_t activate_value; // 进入时的幅度值, 用于判断是否过小

    uint16_t min_width;
    uint16_t max_width;
    uint16_t min_peak;

    uint8_t refractory_cnt;
    uint8_t refractory_len;

    // 输出
    uint8_t event_flag; // 检测到一个完整波
    uint16_t event_peak;
    uint16_t event_width;
    uint32_t event_area;
    
} wave_detector_t;

#define SC7A20_INIT_MAX 100

#define G_THRESHOLD 250.0f

#endif
