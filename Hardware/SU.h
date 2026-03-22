#ifndef __SU__H_
#define __SU__H_

#include "stm32f10x.h"

extern volatile uint8_t SU_data;
extern volatile uint8_t SU_Receive_flag;

void SU_Init(void);

#endif
