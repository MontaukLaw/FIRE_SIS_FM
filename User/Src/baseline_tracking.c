
#include "main.h"

float result_data[TOTAL_SENSOR_NUMBER] = {0}; // 最终结果数据

struct g_baseline_tracking_t g = {0}; // 全局变量，记录基线/噪声/强度/质心等数据

__IO uint8_t c0_status = 0;

wave_detector_t c0_wave_detector = {0};

// 更新g里面的数据
void update_base_line_and_noise(float *sample, uint16_t len, uint16_t base_lpha_num, uint16_t basealpha_den, uint16_t noise_alpha_num, uint16_t noise_alpha_den)
{
    for (uint16_t i = 0; i < len; i++)
    {
        float raw = sample[i];
        g.base_line[i] = ema_float(g.base_line[i], raw, base_lpha_num, basealpha_den);
        float absd = float_asb_diff(raw, g.base_line[i]);
        g.base_noise[i] = ema_float(g.base_noise[i], absd, noise_alpha_num, noise_alpha_den);
    }
}

// ===== 私有：计算各类强度/质心 =====
// 首先将当前值平滑归一化之后
// 计算噪声*K_TH 作为单通道门限
// 把超过门限的值作为“有效信号”
// 然后记录各种超阈值的例如总值, 跟通道的绑定,
static void compute_strength_and_centroid_v3(float *sample)
{

    const uint16_t n = MAX_SENSORS;

    float S_amp = 0, sum_idx_amp = 0;
    float th_acc = 0;
    float S_w = 0, sum_idx_w = 0;
    uint32_t S_norm_q8 = 0;
    uint16_t C_act = 0;
    float raw_now = 0.0f;
    float base = 0.0f;
    float diff_new = 0.0f;
    float th = 0.0f;
    float th_off = 0.0f;
    float over_threshold = 0.0f;

    for (uint16_t i = 0; i < n; i++)
    {
        // 采样值
        raw_now = sample[i];

        // 拿到基线值
        base = g.base_line[i];

        // 计算当前值与基线的差值
        // 且差值不能为负
        diff_new = (raw_now > base) ? (raw_now - base) : 0u;

        // 对diff做一个简单低通
        diff_new = low_filter_op(g.diff_f[i], diff_new, &g.diff_f[i], 0.8f);

        // 记录差值, 下次低通使用
        g.diff_f[i] = diff_new;

        // 单通道门限 th_i, 以自己的噪声作为基准
        th = K_TH_F * g.base_noise[i];

        // 设置以为最小阈值, incase噪声过小导致门限过小
        if (th < TH_MIN_ABS)
        {
            th = TH_MIN_ABS;
        }

        // 离开门限的部分, 用于判断是否离开接触范围
        th_off = K_TH_OFF_F * g.base_noise[i];

        // 阈后强度（幅度制）
        // 即超过阈值的部分
        over_threshold = (diff_new > th) ? (diff_new - th) : 0u;

        // 全部通道的门限
        th_acc += th;

        // 结果就是这个超过阈值的部分.
        g.val_over_threshold[i] = over_threshold;

        // 幅度无关强度：相对超阈比（Q8）+ 活跃通道数
        if (diff_new > th)
        {
            g.over_th[i] = true;
            C_act++;
        }
        else
        {
            g.over_th[i] = false;
        }

        if (i == 0)
        {
        }

        // 全部通道超阈值的和
        g.omiga_sig = S_amp;

        // 平均单通道门限
        g.avg_th = th_acc / n;

        // 活跃通道数
        g.C_act = C_act;

        // 数据准备完毕，更新全局变量
        memcpy(result_data, g.val_over_threshold, sizeof(float) * TOTAL_SENSOR_NUMBER);
    }
}

#if 0
void WaveDetector_Process_ai(wave_detector_t *wd, uint16_t sample, uint16_t baseline)
{
    uint16_t delta;

    wd->event_flag = 0;

    if (sample > baseline)
        delta = sample - baseline;
    else
        delta = 0;

    switch (wd->state)
    {
    case WAVE_IDLE:
        if (delta >= wd->th_on)
        {
            wd->on_count++;
            if (wd->on_count >= wd->on_count_limit)
            {
                wd->state = WAVE_ACTIVE;
                wd->width = 1;
                wd->peak = delta;
                wd->area = delta;
                wd->off_count = 0;
            }
        }
        else
        {
            wd->on_count = 0;
        }
        break;

    case WAVE_ACTIVE:
        wd->width++;

        if (delta > wd->peak)
            wd->peak = delta;

        wd->area += delta;

        if (delta <= wd->th_off)
        {
            wd->off_count++;
        }
        else
        {
            wd->off_count = 0;
        }

        // 满足退出条件，认为波结束
        if (wd->off_count >= wd->off_count_limit)
        {
            if (wd->width >= wd->min_width &&
                wd->width <= wd->max_width &&
                wd->peak >= wd->min_peak)
            {
                wd->event_flag = 1;
                wd->event_peak = wd->peak;
                wd->event_width = wd->width;
                wd->event_area = wd->area;
            }

            wd->state = WAVE_REFRACTORY;
            wd->refractory_cnt = wd->refractory_len;
            wd->on_count = 0;
            wd->off_count = 0;
        }

        // 防止长时间卡在 ACTIVE
        if (wd->width > 200)
        {
            wd->state = WAVE_IDLE;
            wd->on_count = 0;
            wd->off_count = 0;
            wd->event_flag = 1;
            wd->event_peak = wd->peak;
            wd->event_width = wd->width;
            wd->event_area = wd->area;
        }
        break;

    case WAVE_REFRACTORY:
        if (wd->refractory_cnt > 0)
        {
            wd->refractory_cnt--;
        }
        else
        {
            wd->state = WAVE_IDLE;
        }
        break;

    default:
        wd->state = WAVE_IDLE;
        break;
    }
}
#endif

void WaveDetector_Process(wave_detector_t *wd, float diff, uint16_t th_on, uint16_t th_off)
{
    uint16_t delta;

    wd->event_flag = 0;

    if (diff > 65535)
        diff = 65535;
    else if (diff < 0)
        diff = 0;

    delta = (uint16_t)diff;

    switch (wd->state)
    {
    case WAVE_IDLE:
        if (delta >= th_on) // 直接用噪声乘以系数作为门限, 避免门限过小
        {
            wd->on_count++;
            if (wd->on_count >= wd->on_count_limit)
            {
                wd->state = WAVE_ACTIVE;
                wd->width = 1;
                wd->peak = delta;
                wd->area = delta;
                wd->off_count = 0;
                wd->activate_value = th_on;
            }
        }
        else
        {
            wd->on_count = 0;
        }
        break;

    case WAVE_ACTIVE:
        wd->width++;

        if (delta > wd->peak)
            wd->peak = delta;

        wd->area += delta;

        if (delta <= th_off)
        {
            wd->off_count++;
        }
        else
        {
            wd->off_count = 0;
        }

        // 满足退出条件，认为波结束
        if (wd->off_count >= wd->off_count_limit)
        {
            if (wd->width >= wd->min_width &&
                wd->width <= wd->max_width &&
                wd->peak >= wd->min_peak)
            {
                wd->event_flag = 1;
                wd->event_peak = wd->peak;
                wd->event_width = wd->width;
                wd->event_area = wd->area;
            }

            wd->state = WAVE_REFRACTORY;
            wd->refractory_cnt = wd->refractory_len;
            wd->on_count = 0;
            wd->off_count = 0;
        }

        // 防止长时间卡在 ACTIVE
        if (wd->width > 100)
        {
            wd->state = WAVE_IDLE;
            wd->on_count = 0;
            wd->off_count = 0;
            wd->event_flag = 1;
            wd->event_peak = wd->peak;
            wd->event_width = wd->width;
            wd->event_area = wd->area;
        }
        break;

    case WAVE_REFRACTORY:
        if (wd->refractory_cnt > 0)
        {
            wd->refractory_cnt--;
        }
        else
        {
            wd->state = WAVE_IDLE;
        }
        break;

    default:
        wd->state = WAVE_IDLE;
        break;
    }
}

void fast_cali(float *sample)
{
    static uint16_t calib_counter = 0;

    if (g.initialized)
        return;

    update_base_line_and_noise(sample, MAX_CH_NMB, 1, 4, 1, 4);

    // INIT_ZERO_TIMES用于确定基线大致范围
    calib_counter++;
    if (calib_counter > INIT_ZERO_TIMES)
    {
        g.initialized = true;
        g.thresholds_ready = false;
    }
}

static void maybe_update_thresholds_once(void)
{
    if (g.thresholds_ready)
        return;

    // 幅度制门槛
    g.s_on = g.avg_th * SON_NUM / SON_DEN;

    g.s_hold = (g.s_on * HOLD_NUM) / HOLD_DEN;
    g.s_off = (g.s_on * OFF_NUM) / OFF_DEN;

    // 幅度无关门槛（常数起步，可按需要改为自适应）
    g.s_on_norm = S_NORM_ON;
    g.s_hold_norm = (S_NORM_ON * S_NORM_HOLD_NUM) / S_NORM_HOLD_DEN;
    g.s_off_norm = (S_NORM_ON * S_NORM_OFF_NUM) / S_NORM_OFF_DEN;

    g.c_on = C_ON;
    g.c_hold = C_HOLD;
    g.c_off = C_OFF;

    g.thresholds_ready = true;
    // printf("g.avg_th: %f s_on: %f off: %f\r\n", g.avg_th, g.s_on, g.s_off);

    // printf("Thresholds set: S on/hold/off = %u/%u/%u; S_norm_q8 on/hold/off = %u/%u/%u; C on/hold/off = %u/%u/%u\r\n",
    //        g.s_on, g.s_hold, g.s_off,
    //        g.s_on_norm, g.s_hold_norm, g.s_off_norm,
    //        g.c_on, g.c_hold, g.c_off);
}

void baseline_update_quitely(float *sample)
{

    if (!g.initialized)
        return;

    for (int i = 0; i < MAX_SENSORS; i++)
    {
        if (g.over_th[i])
        {
            g.quiet_up_cnt[i] = 0;
            g.over_th_cnt[i]++;
            // 超过2秒
            if (g.over_th_cnt[i] > OVER_TH_TIMES)
            {
                // 开始跟随
                g.base_line[i] = ema_float(g.base_line[i], sample[i], ACTIVATE_BASE_BETA_NUM, ACTIVATE_BASE_BETA_DEN);
                float absd = float_asb_diff(sample[i], g.base_line[i]);
                g.base_noise[i] = ema_float(g.base_noise[i], absd, ACTIVATE_NOISE_BETA_NUM, ACTIVATE_NOISE_BETA_DEN);

                if (g.base_noise[i] < NOISE_MIN)
                {
                    g.base_noise[i] = NOISE_MIN;
                }
            }
        }
        else
        {
            g.over_th_cnt[i] = 0;
            g.quiet_up_cnt[i]++;

            // 安静状态下跟随
            if (g.quiet_up_cnt[i] > QUITE_TIMES)
            {
                // printf("Quiet up: %d\r\n", i);
                // 1/100

                g.base_line[i] = ema_float(g.base_line[i], sample[i], BASE_BETA_NUM, BASE_BETA_DEN);

                // if (g.start_save_baseline == 0)
                // {
                //     if (g.base_line[i] < g_baseline_info.base_line_in_flash[i])
                //     {
                //         g.base_line[i] = g_baseline_info.base_line_in_flash[i];
                //     }
                // }

                float absd = float_asb_diff(sample[i], g.base_line[i]);
                g.base_noise[i] = ema_float(g.base_noise[i], absd, NOISE_IDEL_NUM, NOISE_IDEL_DEN);
                // 为避免这个值太小导致门限过小, 可以设置一个下限
                if (g.base_noise[i] < NOISE_MIN)
                {
                    g.base_noise[i] = NOISE_MIN;
                }
            }
        }
    }
}

void baseline_tracking(float *sample)
{
    // 记录初始化之前的基线和噪声数据
    fast_cali(sample);

    compute_strength_and_centroid_v3(sample);

    // 一次性更新门槛
    maybe_update_thresholds_once();

    // 空闲时更新基线与噪声
    baseline_update_quitely(sample);
}

void reset_data(void)
{

    memset(&g, 0, sizeof(g));
    memset(result_data, 0, sizeof(result_data));
}

void WaveDetector_Init(wave_detector_t *wd)
{
    wd->state = WAVE_IDLE;

    // wd->th_on = 3; // 根据实际调
    // wd->th_off = 1;

    wd->on_count = 0;
    wd->off_count = 0;

    wd->on_count_limit = 5;
    wd->off_count_limit = 5;

    wd->width = 0;
    wd->peak = 0;
    wd->area = 0;

    wd->min_width = 8;
    wd->max_width = 80;
    wd->min_peak = 4;

    wd->refractory_cnt = 0;
    wd->refractory_len = 6;

    wd->event_flag = 0;
    wd->event_peak = 0;
    wd->event_width = 0;
    wd->event_area = 0;
}
