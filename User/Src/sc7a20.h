#ifndef _SC7A20_H_
#define _SC7A20_H_

void sc7a20_read_acc(int16_t *acc_data, float* combin_acc);

void sc7a20_init(void);

void init_gsensor_for_interrupt(void);

extern int16_t sc7a20_acc_data[];
extern int16_t sc7a20_delta_axes[3];
extern float sc7a20_motion_delta;

#endif
