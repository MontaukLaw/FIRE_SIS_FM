#ifndef _VOICE_APP_H_
#define _VOICE_APP_H_

void play_voice_no_repeat(uint8_t voice_idx);

void play_voice_base_result_data(float result);

void play_voice_base_wave_detector(wave_detector_t wd);

void play_voice_base_g(float combin_acc);

#endif 
