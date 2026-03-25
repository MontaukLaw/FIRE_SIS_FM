#ifndef __ARITHMETIC_H
#define __ARITHMETIC_H

#include "py32f0xx_hal.h"

// 结构体类型定义
// typedef struct 
// {
//     float LastP;    //上次估算协方差 初始化值为0.02
//     float Now_P;    //当前估算协方差 初始化值为0
//     float out;      //卡尔曼滤波器输出 初始化值为0
//     float Kg;       //卡尔曼增益 初始化值为0
//     float Q;        //过程噪声协方差 初始化值为0.001
//     float R;        //观测噪声协方差 初始化值为0.543
// }KFP;               //Kalman Filter parameter

float averageFilter(float in_data, int channel);
float weightedAverageFilter(float in_data, int channel);
float middleFilter(float in_data, int channel);
float low_pass_filter(float x, float prev_y, float alpha);
float high_pass_filter(float x, float prev_x, float prev_y, float alpha);
float filter1(float in_data, int channel);
// float kalmanFilter(KFP *kfp, float input);

// extern volatile float pre_low_value[4] ;
// extern volatile float pre_high_value[4];
// extern volatile float pre_high_sig[4];
// extern KFP KFP_value;
    
#endif 

