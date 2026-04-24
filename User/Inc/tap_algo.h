#ifndef _TAP_ALGO_H_
#define _TAP_ALGO_H_

#include <stdbool.h>
#include <stdint.h>

/* 采样率假设为 200Hz。 */
#define TAP_FS_HZ 200U

/* 时间窗口参数。 */
#define SHORT_WIN 12U
#define LONG_WIN 80U
#define G_DELAY_WIN 40U
#define CANDIDATE_WIN 12U
#define REFRACTORY_WIN 40U
#define FLIP_INHIBIT_WIN 80U

/* 峰宽约束。 */
#define PEAK_MIN_WIDTH 4U
#define PEAK_MAX_WIDTH 16U

/* Q8 滤波系数。 */
#define ALPHA_PRE_Q8 92U
#define ALPHA_G_Q8 12U

/* 触发阈值。 */
#define TH_DYN2 6400UL
#define TH_JERK2 225UL
#define TH_E_SHORT 12000UL
#define TH_RATIO_X1000 200UL
#define WIDTH_LEVEL2 1225UL

/* 抬起抑制 */
#define TH_DYN2_RISE (300*300)

/* 翻板/姿态变化相关阈值。
 * 这两个值和传感器单位、安装姿态有关，先给一个保守初值，后续可再调。 */
#define TH_GMOVE_LOW 120U
#define TH_GMOVE_HIGH 240U
#define TH_FLIP_DYN2 57600UL

typedef enum
{
    TAP_IDLE = 0,
    TAP_CANDIDATE,
    TAP_REFRACTORY
} TapState;

typedef struct
{
    /* 轻微预滤波后的值，Q8 定点。 */
    int32_t xf_q8;
    int32_t yf_q8;
    int32_t zf_q8;

    /* 慢速重力估计，Q8 定点。 */
    int32_t gx_q8;
    int32_t gy_q8;
    int32_t gz_q8;

    /* 上一个采样点的动态分量。 */
    int16_t dx_prev;
    int16_t dy_prev;
    int16_t dz_prev;

    /* 短窗 / 长窗能量缓存。 */
    uint32_t short_buf[SHORT_WIN];
    uint32_t long_buf[LONG_WIN];
    uint32_t short_sum;
    uint32_t long_sum;
    uint8_t short_idx;
    uint8_t long_idx;

    /* 用延迟重力历史估计姿态变化。 */
    int16_t gx_hist[G_DELAY_WIN];
    int16_t gy_hist[G_DELAY_WIN];
    int16_t gz_hist[G_DELAY_WIN];
    uint8_t g_idx;

    /* 状态机相关变量。 */
    uint8_t state;
    uint8_t candidate_cnt;
    uint8_t refractory_cnt;
    uint8_t flip_inhibit_cnt;

    /* 候选窗口内统计量。 */
    uint32_t cand_peak_dyn2;
    uint32_t cand_peak_jerk2;
    uint8_t cand_width;

    /* 调试量输出。 */
    uint32_t dyn2;
    uint32_t jerk2;
    uint32_t e_short;
    uint32_t e_long;
    uint16_t gmove;
    uint32_t tap_count;

    uint8_t initialized;
} TapCtx;

const char *Tap_StateName(uint8_t state);
void Tap_Init(TapCtx *ctx);
bool Tap_Process(TapCtx *ctx, int16_t x_raw, int16_t y_raw, int16_t z_raw);

#endif
