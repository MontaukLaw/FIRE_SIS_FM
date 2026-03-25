#ifndef __IWDG_H
#define __IWDG_H

#include "py32f0xx_hal.h"

extern IWDG_HandleTypeDef IwdgHandle; // 看门狗句柄
void IWDG_Init(void);
void feed_dog(void);

#endif
