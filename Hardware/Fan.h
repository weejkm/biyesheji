#ifndef __FAN_H
#define __FAN_H

/**
 ******************************************************************************
 * @file    Fan.h
 * @brief   直流电机（风扇）驱动头文件
 * @details 本文件提供了风扇的初始化和开启/关闭控制函数
 *          风扇连接在PB8引脚，通过GPIO控制风扇的开关
 ******************************************************************************
 */

#include "stm32f10x.h"

// ==================== 风扇引脚定义 ====================

/**
 * @brief 直流电机（风扇）引脚定义
 * @note 风扇连接在PB8引脚，通过GPIO电平控制风扇开关
 */
#define Fan_GPIO_PORT       GPIOB           // GPIOB端口
#define Fan_GPIO_PIN        GPIO_Pin_8      // PB8引脚
#define Fan_GPIO_CLK        RCC_APB2Periph_GPIOB  // GPIOB时钟

// ==================== 函数声明 ====================

/**
 * @brief 初始化风扇驱动
 * @note 配置PB8为推挽输出模式，默认输出高电平（风扇关闭）
 */
void Fan_Init(void);

/**
 * @brief 设置风扇状态（开/关）
 * @param STAT 风扇状态：0-关闭，1-开启
 * @note 0: 输出高电平，风扇关闭
 *       1: 输出低电平，风扇开启
 */
void Fan_set(uint8_t STAT);

#endif /* __FAN_H */
