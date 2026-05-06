#ifndef _SIM_I2C_H_
#define _SIM_I2C_H_

void init_gsensor_for_lp_int_sim(void);

void init_sim_i2c_gpio(void);

void sc7a20_get_data_sim(int16_t *xyz_val);

void deinit_gsensor_gpio(void);

void init_gsenor_sim(void);

#endif /* _SIM_I2C_H_ */
