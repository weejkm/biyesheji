#ifndef __FAN_H
#define __FAN_H

#include "stm32f10x.h" // 根据你的芯片型号包含对应的头文件

//风扇引脚配置
#define Fan_GPIO_PORT       GPIOB
#define Fan_GPIO_PIN        GPIO_Pin_8
#define Fan_GPIO_CLK        RCC_APB2Periph_GPIOB

// 函数声明
void Fan_Init(void);
void Fan_set(uint8_t STAT);

#endif /* __FAN_H */
