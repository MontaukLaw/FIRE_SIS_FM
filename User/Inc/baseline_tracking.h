#ifndef _BASELINE_TRACKING_H_
#define _BASELINE_TRACKING_H_

#include "user_define.h"

void baseline_tracking(float *sample);

extern float result_data[];

extern struct g_baseline_tracking_t g;

void reset_data(void);

void wave_detector_init(wave_detector_t *wd);

void WaveDetector_Process_ai(wave_detector_t *wd, uint16_t sample, uint16_t baseline);

// void WaveDetector_Process(wave_detector_t *wd, float diff);
void wave_detector_process(wave_detector_t *wd, float diff, uint16_t th_on, uint16_t th_off);

extern wave_detector_t c0_wave_detector;

#endif
