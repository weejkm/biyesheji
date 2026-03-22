#ifndef __LED_H
#define __LED_H

/**
 ******************************************************************************
 * @file    LED.h
 * @brief   LED控制驱动头文件
 * @details 本文件提供了LED初始化和亮度控制函数
 *          使用PWM方式进行亮度调节，支持0-25级亮度
 ******************************************************************************
 */

#include "stm32f10x.h"

// ==================== 引脚定义 ====================

/**
 * @brief LED引脚定义
 * @note LED连接在PA11引脚，使用TIM1的通道4输出PWM
 */
#define LED1_PIN GPIO_PIN_11

// ==================== 函数声明 ====================

/**
 * @brief 初始化LED驱动
 * @note 配置PA11为TIM1_CH4的PWM输出模式
 *       PWM频率约为1kHz，支持0-25级亮度调节
 */
void LED_Init(void);

/**
 * @brief 设置LED亮度
 * @param light 亮度值，范围0-25
 *        - 0: 关闭灯
 *        - 1-25: 亮度递增
 * @note 实际PWM占空比 = light * 10 / 1000
 */
void LED_Set_light(uint8_t light);

#endif
