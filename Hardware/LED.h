#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"                  // Device header


#define LED1_PIN GPIO_PIN_11


void LED_Init(void);
void LED_Set_light(uint8_t light);


#endif
