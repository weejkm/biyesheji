#ifndef __DELAY_H__
#define __DELAY_H__

#include "stm32f10x.h"                  // Device header


void Delay_us(uint32_t us);
void delay_us(uint32_t us);

void Delay_ms(uint32_t ms);
void delay_ms(uint32_t ms);

void Timer_Init(void);
void delay_init(void);

#endif
