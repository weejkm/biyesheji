#ifndef __LIGHT_H__
#define __LIGHT_H__


#include "stm32f10x.h"
#include "light.h"

#define LIGHT_GPIO GPIOA
#define LIGHT_GPIO_PIN GPIO_Pin_1
#define LIGHT_GPIO_CLK RCC_APB2Periph_GPIOA

void Init_GPIO(void);

void Init_ADC1(void);

uint16_t Read_ADC_Value(void);





#endif
