#include "user_define.h"
#include "sc7a20.h"
#include "tim.h"
#include "usart.h"

uint32_t exti_triggered_ts = 0;

uint32_t last_play_ts = 0;

static int16_t abs_i16(int16_t value)
{
    return (value < 0) ? (int16_t)(-value) : value;
}

static uint16_t calc_weighted_motion(int16_t dx, int16_t dy, int16_t dz)
{
#if (GSENSOR_AXIS_MODE == GSENSOR_AXIS_MODE_Z_ONLY)
    (void)dx;
    (void)dy;
    return (uint16_t)abs_i16(dz);
#else
    return (uint16_t)((abs_i16(dx) * GSENSOR_AXIS_WEIGHT_X +
                       abs_i16(dy) * GSENSOR_AXIS_WEIGHT_Y +
                       abs_i16(dz) * GSENSOR_AXIS_WEIGHT_Z) /
                      100U);
#endif
}

void play_one_pulse(void)
{
    HAL_GPIO_WritePin(VOICE_PORT, VOICE_PIN, GPIO_PIN_SET);
    delay_100us();
    HAL_GPIO_WritePin(VOICE_PORT, VOICE_PIN, GPIO_PIN_RESET);
    delay_100us();
}

void play_voice(uint8_t voice_index)
{
    play_one_pulse();

    for (uint8_t i = 0; i < voice_index; i++)
    {
        play_one_pulse();
    }
}

void play_voice_no_repeat(uint8_t voice_index)
{
    static uint8_t last_play = 0;

    if (last_play == voice_index)
    {
        voice_index = (voice_index % 3) + 1;
    }
    last_play = voice_index;

    play_one_pulse();

    for (uint8_t i = 0; i < voice_index; i++)
    {
        play_one_pulse();
    }

    last_play_ts = HAL_GetTick();

    last_active_ts = HAL_GetTick();
}

void play_level1(void)
{
    play_voice_no_repeat((uint8_t)(rand() % 3 + 1));
}

void play_level2(void)
{
    play_voice_no_repeat((uint8_t)(rand() % 3 + 4));
}

void play_level3(void)
{
    play_voice_no_repeat((uint8_t)(rand() % 3 + 7));
}

void play_voice_base_result_data(float result)
{
    static uint32_t last_run_ts = 0;

    if (HAL_GetTick() - last_run_ts < 200)
    {
        return;
    }

    if (HAL_GetTick() - last_play_ts < 2000)
    {
        return;
    }

    if (result > LEVEL_1 && result < LEVEL_2)
    {
        play_voice_no_repeat((uint8_t)(rand() % 3 + 1));
    }
    else if (result >= LEVEL_2 && result < LEVEL_3)
    {
        play_voice_no_repeat((uint8_t)(rand() % 3 + 4));
    }
    else if (result >= LEVEL_3)
    {
        play_voice_no_repeat((uint8_t)(rand() % 3 + 7));
    }

    last_run_ts = HAL_GetTick();
}

void play_voice_base_g(float combin_acc)
{
    static uint32_t last_play_ts_local = 0;

    if (HAL_GetTick() - last_play_ts_local < 2000)
    {
        return;
    }

    if (combin_acc > G_THRESHOLD)
    {
        play_voice(8);
        last_play_ts_local = HAL_GetTick();
    }
}

void play_voice_base_wave_detector(wave_detector_t wd)
{
    static uint32_t last_run_ts = 0;

    if (!wd.event_flag)
    {
        return;
    }

    if (wd.event_peak < LEVEL_2)
    {
        play_level1();
    }
    else if (wd.event_peak < LEVEL_3)
    {
        play_level2();
    }
    else
    {
        play_level3();
    }

    last_run_ts = HAL_GetTick();
    last_play_ts = HAL_GetTick();

    (void)last_run_ts;
}

void simple_shake_and_play_task(void)
{
    if (shaked == 0)
        return;

    if (HAL_GetTick() < last_play_ts + 1000)
        return;

    // play_voice(1);

    last_play_ts = HAL_GetTick();

    last_active_ts = HAL_GetTick();

    shaked = 0;
}

void shake_and_play_task(void)
{
    static uint32_t last_shake_play_ts = 0;
    static uint16_t feature_energy = 0;
    static uint8_t feature_active_samples = 0;
    static int16_t peak_abs_dx = 0;
    static int16_t peak_abs_dy = 0;
    static int16_t peak_abs_dz = 0;
    static int16_t peak_pos_dx = 0;
    static int16_t peak_pos_dy = 0;
    static int16_t peak_pos_dz = 0;
    static int16_t peak_neg_dx = 0;
    static int16_t peak_neg_dy = 0;
    static int16_t peak_neg_dz = 0;

    if (!shaked)
    {
        return;
    }

#if GSENSOR_EVENT_DEBUG_PRINTF
    printf("INT\r\n");
#endif

    {
        uint32_t now = HAL_GetTick();
        uint8_t fail_mask = 0;
        bool success = false;
        bool rebound_detected = false;
        int16_t peak_any;
        uint16_t weighted_motion;

        /* Too close to the last accepted event: discard this one immediately. */
        if ((now - last_shake_play_ts) < GSENSOR_INT_PLAY_COOLDOWN_MS)
        {
            uart_send_gsensor_event(0U, GSENSOR_FAIL_COOLDOWN, 0, 0, 0, 0U);
            shaked = 0;
            return;
        }

        /* Update the largest absolute delta seen on each axis in this event window. */
        if (abs_i16(sc7a20_delta_axes[0]) > peak_abs_dx)
        {
            peak_abs_dx = abs_i16(sc7a20_delta_axes[0]);
        }
        if (abs_i16(sc7a20_delta_axes[1]) > peak_abs_dy)
        {
            peak_abs_dy = abs_i16(sc7a20_delta_axes[1]);
        }
        if (abs_i16(sc7a20_delta_axes[2]) > peak_abs_dz)
        {
            peak_abs_dz = abs_i16(sc7a20_delta_axes[2]);
        }

        /* Track the strongest positive and negative swing on each axis. */
        if (sc7a20_delta_axes[0] > peak_pos_dx)
        {
            peak_pos_dx = sc7a20_delta_axes[0];
        }
        if (sc7a20_delta_axes[1] > peak_pos_dy)
        {
            peak_pos_dy = sc7a20_delta_axes[1];
        }
        if (sc7a20_delta_axes[2] > peak_pos_dz)
        {
            peak_pos_dz = sc7a20_delta_axes[2];
        }
        if (sc7a20_delta_axes[0] < peak_neg_dx)
        {
            peak_neg_dx = sc7a20_delta_axes[0];
        }
        if (sc7a20_delta_axes[1] < peak_neg_dy)
        {
            peak_neg_dy = sc7a20_delta_axes[1];
        }
        if (sc7a20_delta_axes[2] < peak_neg_dz)
        {
            peak_neg_dz = sc7a20_delta_axes[2];
        }

        weighted_motion = calc_weighted_motion(sc7a20_delta_axes[0],
                                               sc7a20_delta_axes[1],
                                               sc7a20_delta_axes[2]);

        /* Accumulate short-window "energy" with Z-axis priority. */
        feature_energy += weighted_motion;

        /* Count how many samples are truly active after axis weighting. */
        if (weighted_motion >= GSENSOR_FEATURE_ACTIVE_TH)
        {
            feature_active_samples++;
        }

        /* Keep collecting until the confirmation window expires. */
        if ((now - exti_triggered_ts) < GSENSOR_CONFIRM_WINDOW_MS)
        {
            return;
        }

        /* Final acceptance must come from Z. X/Y only help as auxiliary energy. */
        peak_any = peak_abs_dz;

        /* Rebound also uses Z only, so side-axis motion cannot fake a tap. */
        rebound_detected =
            (peak_pos_dz >= GSENSOR_FEATURE_REBOUND_TH) &&
            (peak_neg_dz <= -GSENSOR_FEATURE_REBOUND_TH);

        /* Build a bitmask that says exactly which checks failed. */
        if (peak_any < GSENSOR_FEATURE_PEAK_TH)
        {
            fail_mask |= GSENSOR_FAIL_PEAK;
        }
        if (feature_energy < GSENSOR_FEATURE_ENERGY_TH)
        {
            fail_mask |= GSENSOR_FAIL_ENERGY;
        }
        if (feature_active_samples < GSENSOR_FEATURE_ACTIVE_COUNT)
        {
            fail_mask |= GSENSOR_FAIL_ACTIVE;
        }
        if (!rebound_detected)
        {
            fail_mask |= GSENSOR_FAIL_REBOUND;
        }

        if (fail_mask == 0)
        {
            // play_level1();
            play_voice(1);
            last_shake_play_ts = now;
            success = true;
        }

        /* Report both success/failure and the measured feature values to the host. */
        uart_send_gsensor_event(success ? 1U : 0U,
                                fail_mask,
                                peak_any,
                                feature_energy,
                                feature_active_samples,
                                rebound_detected ? 1U : 0U);

#if GSENSOR_EVENT_DEBUG_PRINTF
        printf("ok=%u fail=0x%02X peak=%d energy=%u active=%u rebound=%u\r\n",
               success ? 1 : 0,
               fail_mask,
               peak_any,
               feature_energy,
               feature_active_samples,
               rebound_detected ? 1 : 0);
#endif

        /* Reset all window statistics for the next interrupt-driven event. */
        feature_energy = 0;
        feature_active_samples = 0;
        peak_abs_dx = 0;
        peak_abs_dy = 0;
        peak_abs_dz = 0;
        peak_pos_dx = 0;
        peak_pos_dy = 0;
        peak_pos_dz = 0;
        peak_neg_dx = 0;
        peak_neg_dy = 0;
        peak_neg_dz = 0;
        shaked = 0;
    }
}
