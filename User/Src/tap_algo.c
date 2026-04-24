#include "tap_algo.h"

#include <string.h>

static int16_t abs_i16(int16_t value)
{
    return (value < 0) ? (int16_t)(-value) : value;
}

static void tap_init_filters(TapCtx *ctx, int16_t x_raw, int16_t y_raw, int16_t z_raw)
{
    int16_t gx_now = x_raw;
    int16_t gy_now = y_raw;
    int16_t gz_now = z_raw;

    ctx->xf_q8 = ((int32_t)x_raw) << 8;
    ctx->yf_q8 = ((int32_t)y_raw) << 8;
    ctx->zf_q8 = ((int32_t)z_raw) << 8;

    ctx->gx_q8 = ((int32_t)x_raw) << 8;
    ctx->gy_q8 = ((int32_t)y_raw) << 8;
    ctx->gz_q8 = ((int32_t)z_raw) << 8;

    for (uint8_t i = 0; i < G_DELAY_WIN; i++)
    {
        ctx->gx_hist[i] = gx_now;
        ctx->gy_hist[i] = gy_now;
        ctx->gz_hist[i] = gz_now;
    }

    ctx->initialized = 1U;
}

static void tap_apply_filters(TapCtx *ctx,
                              int16_t x_raw,
                              int16_t y_raw,
                              int16_t z_raw,
                              int16_t *dx,
                              int16_t *dy,
                              int16_t *dz)
{
    int32_t x_q8 = ((int32_t)x_raw) << 8;
    int32_t y_q8 = ((int32_t)y_raw) << 8;
    int32_t z_q8 = ((int32_t)z_raw) << 8;
    int32_t dx_q8;
    int32_t dy_q8;
    int32_t dz_q8;

    /* 轻微预滤波：压毛刺，但尽量保留拍打主峰。 */
    ctx->xf_q8 += (int32_t)(((x_q8 - ctx->xf_q8) * (int32_t)ALPHA_PRE_Q8) >> 8);
    ctx->yf_q8 += (int32_t)(((y_q8 - ctx->yf_q8) * (int32_t)ALPHA_PRE_Q8) >> 8);
    ctx->zf_q8 += (int32_t)(((z_q8 - ctx->zf_q8) * (int32_t)ALPHA_PRE_Q8) >> 8);

    /* 慢速重力估计：主要跟踪姿态和重力方向。 */
    ctx->gx_q8 += (int32_t)(((ctx->xf_q8 - ctx->gx_q8) * (int32_t)ALPHA_G_Q8) >> 8);
    ctx->gy_q8 += (int32_t)(((ctx->yf_q8 - ctx->gy_q8) * (int32_t)ALPHA_G_Q8) >> 8);
    ctx->gz_q8 += (int32_t)(((ctx->zf_q8 - ctx->gz_q8) * (int32_t)ALPHA_G_Q8) >> 8);

    /* 动态分量 = 预滤波值 - 慢速重力估计。 */
    dx_q8 = ctx->xf_q8 - ctx->gx_q8;
    dy_q8 = ctx->yf_q8 - ctx->gy_q8;
    dz_q8 = ctx->zf_q8 - ctx->gz_q8;

    *dx = (int16_t)(dx_q8 >> 8);
    *dy = (int16_t)(dy_q8 >> 8);
    *dz = (int16_t)(dz_q8 >> 8);
}

static void tap_compute_features(TapCtx *ctx,
                                 int16_t dx,
                                 int16_t dy,
                                 int16_t dz,
                                 uint32_t *dyn2,
                                 uint32_t *jerk2)
{
    int16_t jx = (int16_t)(dx - ctx->dx_prev);
    int16_t jy = (int16_t)(dy - ctx->dy_prev);
    int16_t jz = (int16_t)(dz - ctx->dz_prev);

    /* 当前冲击强度。 */
    *dyn2 = (uint32_t)((int32_t)dx * dx) +
            (uint32_t)((int32_t)dy * dy) +
            (uint32_t)((int32_t)dz * dz);

    /* 当前突变强度。 */
    *jerk2 = (uint32_t)((int32_t)jx * jx) +
             (uint32_t)((int32_t)jy * jy) +
             (uint32_t)((int32_t)jz * jz);

    ctx->dx_prev = dx;
    ctx->dy_prev = dy;
    ctx->dz_prev = dz;
}

static void tap_update_window(uint32_t *sum,
                              uint32_t *buf,
                              uint8_t *idx,
                              uint8_t win,
                              uint32_t value)
{
    *sum -= buf[*idx];
    *sum += value;
    buf[*idx] = value;

    (*idx)++;
    if (*idx >= win)
    {
        *idx = 0;
    }
}

static uint16_t tap_update_gmove(TapCtx *ctx)
{
    int16_t gx_now = (int16_t)(ctx->gx_q8 >> 8);
    int16_t gy_now = (int16_t)(ctx->gy_q8 >> 8);
    int16_t gz_now = (int16_t)(ctx->gz_q8 >> 8);
    int16_t gx_old = ctx->gx_hist[ctx->g_idx];
    int16_t gy_old = ctx->gy_hist[ctx->g_idx];
    int16_t gz_old = ctx->gz_hist[ctx->g_idx];
    uint16_t gmove;

    /* 200ms 前后重力方向的变化量：大则更像翻板/拿起。 */
    gmove = (uint16_t)(abs_i16((int16_t)(gx_now - gx_old)) +
                       abs_i16((int16_t)(gy_now - gy_old)) +
                       abs_i16((int16_t)(gz_now - gz_old)));

    ctx->gx_hist[ctx->g_idx] = gx_now;
    ctx->gy_hist[ctx->g_idx] = gy_now;
    ctx->gz_hist[ctx->g_idx] = gz_now;

    ctx->g_idx++;
    if (ctx->g_idx >= G_DELAY_WIN)
    {
        ctx->g_idx = 0;
    }

    return gmove;
}

const char *Tap_StateName(uint8_t state)
{
    switch (state)
    {
    case TAP_IDLE:
        return "IDLE";
    case TAP_CANDIDATE:
        return "CAND";
    case TAP_REFRACTORY:
        return "REF";
    default:
        return "UNK";
    }
}

void Tap_Init(TapCtx *ctx)
{
    memset(ctx, 0, sizeof(TapCtx));
    ctx->state = TAP_IDLE;
}

bool Tap_Process(TapCtx *ctx, int16_t x_raw, int16_t y_raw, int16_t z_raw)
{
    bool triggered = false;
    int16_t dx;
    int16_t dy;
    int16_t dz;
    uint32_t dyn2;
    uint32_t jerk2;
    uint16_t gmove;

    /* 第一次进入时还没有历史数据，只做滤波器初始化，不做判定。 */
    if (!ctx->initialized)
    {
        tap_init_filters(ctx, x_raw, y_raw, z_raw);
        return false;
    }

    /* 1) 更新滤波器，并得到去重力后的动态分量。 */
    tap_apply_filters(ctx, x_raw, y_raw, z_raw, &dx, &dy, &dz);

    /* 2) 计算本拍的冲击强度 dyn2 和突变强度 jerk2。 */
    tap_compute_features(ctx, dx, dy, dz, &dyn2, &jerk2);

    /* 3) 更新短窗12周期/长窗80周期能量。
     * short_sum 更关注最近几十毫秒的集中动作，
     * long_sum 更像背景动作强度。400ms */
    tap_update_window(&ctx->short_sum, ctx->short_buf, &ctx->short_idx, SHORT_WIN, dyn2);
    tap_update_window(&ctx->long_sum, ctx->long_buf, &ctx->long_idx, LONG_WIN, dyn2);

    /* 4) 更新姿态变化指标 gmove，用来区分拍打和翻板。 */
    // gmove是跟200ms前后的重力方向变化量的abs的和
    gmove = tap_update_gmove(ctx);

    /* 5) flip inhibit：
     * 如果姿态变化很大，但冲击强度又不够大，更像翻板/拿起，于是开启抑制窗口。 */
    if ((gmove > TH_GMOVE_HIGH) && (dyn2 < TH_FLIP_DYN2))
    {
        ctx->flip_inhibit_cnt = FLIP_INHIBIT_WIN;
    }

    if (ctx->flip_inhibit_cnt > 0U)
    {
        ctx->flip_inhibit_cnt--;
    }

    /* 6) 保存调试量，方便主循环直接打印内部状态。 */
    ctx->dyn2 = dyn2;              // 当前拍的冲击强度
    ctx->jerk2 = jerk2;            // 当前拍的突变速度
    ctx->e_short = ctx->short_sum; // 短窗80ms内的能量
    ctx->e_long = ctx->long_sum;   // 长窗400ms内的能量
    ctx->gmove = gmove;            // 200ms前后重力方向的变化量，区分拍打和翻板

    /* 7) 状态机。 */
    switch (ctx->state)
    {
    case TAP_IDLE:
        /* 空闲态：
         * 只有同时满足“够猛、够脆、短窗能量够、足够突然、姿态变化不大、当前没被翻板抑制”
         * 才进入候选态。 */
        if ((ctx->flip_inhibit_cnt == 0U) &&
            (dyn2 > TH_DYN2) &&
            (jerk2 > TH_JERK2) &&
            (ctx->short_sum > TH_E_SHORT) &&
            (ctx->short_sum * 1000UL > TH_RATIO_X1000 * ctx->long_sum) &&
            (gmove < TH_GMOVE_LOW))
        {
            ctx->state = TAP_CANDIDATE;
            ctx->candidate_cnt = CANDIDATE_WIN;

            /* 进入候选态后，先记下当前这拍的峰值。 */
            ctx->cand_peak_dyn2 = dyn2;
            ctx->cand_peak_jerk2 = jerk2;

            /* 如果当前这一拍已经超过“高能区”门槛，就把峰宽记为 1。 */
            ctx->cand_width = (dyn2 > WIDTH_LEVEL2) ? 1U : 0U;
        }
        break;

    case TAP_CANDIDATE:
        /* 候选态：
         * 在一个短窗口里继续观察，统计峰值和峰宽。 */
        if (dyn2 > ctx->cand_peak_dyn2)
        {
            // 刷新候选窗口内的dyn2峰值
            ctx->cand_peak_dyn2 = dyn2;
        }
        if (jerk2 > ctx->cand_peak_jerk2)
        {
            // 刷新候选窗口内的jerk2峰值
            ctx->cand_peak_jerk2 = jerk2;
        }
        if (dyn2 > WIDTH_LEVEL2)
        {
            // 记录超过高能区门槛的采样点数量，作为峰宽的指标
            ctx->cand_width++;
        }

        if (dyn2 > TH_DYN2_RISE)
        {
            // 超过抬起抑制阈值，直接回到空闲态
            ctx->state = TAP_IDLE;
            break;
        }

        /* 候选期间如果姿态变化突然过大，说明更像翻板，直接取消。 */
        if (gmove > TH_GMOVE_HIGH)
        {
            ctx->state = TAP_IDLE;
            break;
        }

        if (ctx->candidate_cnt > 0U)
        {
            // 控制候选窗口的长度，超过这个窗口还没有满足条件就放弃这次拍打
            ctx->candidate_cnt--;
        }

        /* 候选窗口结束时做最终确认。 */
        if (ctx->candidate_cnt == 0U)
        {
            // 查看在整个候选窗口内的统计量是否满足条件，满足则确认一次拍打
            if ((ctx->cand_peak_dyn2 > TH_DYN2) &&                            // 峰值是否够猛
                (ctx->cand_peak_jerk2 > TH_JERK2) &&                          // 变化是否够突然
                (ctx->short_sum > TH_E_SHORT) &&                              // 短窗能量是否够大
                (ctx->short_sum * 1000UL > TH_RATIO_X1000 * ctx->long_sum) && // 短窗能量与长窗能量的比值是否够大
                (gmove < TH_GMOVE_LOW) &&                                     // 姿态变化是否不大
                (ctx->cand_width >= PEAK_MIN_WIDTH) &&                        // 峰宽是否够宽, 即超过高能区门槛的采样点数量是否足够多
                (ctx->cand_width <= PEAK_MAX_WIDTH))                          // 峰宽是否不过宽, 太宽可能是非拍打的动作
            {
                /* 全部条件满足，确认一次拍打，并进入不应期。 */
                triggered = true;
                ctx->tap_count++;
                ctx->state = TAP_REFRACTORY;          // 进入不应期，避免余振被误判为多次拍打
                ctx->refractory_cnt = REFRACTORY_WIN; // 40周期即200ms的不应期
            }
            else
            {
                /* 观察下来不够像拍打，回到空闲态。 */
                ctx->state = TAP_IDLE;
            }
        }
        break;

    case TAP_REFRACTORY:
        /* 不应期：
         * 避免一次拍打后的余振被当成多次拍打。 */
        if (ctx->refractory_cnt > 0U)
        {
            ctx->refractory_cnt--;
        }

        if (ctx->refractory_cnt == 0U)
        {
            ctx->state = TAP_IDLE;
        }
        break;

    default:
        ctx->state = TAP_IDLE;
        break;
    }

    return triggered;
}
