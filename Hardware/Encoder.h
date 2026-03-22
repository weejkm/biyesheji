#ifndef __ENCODER_H
#define __ENCODER_H

#include "stdint.h"

extern int16_t Encoder_Count;

void Encoder_Init(void);
int16_t Encoder_Get(void);

#endif
