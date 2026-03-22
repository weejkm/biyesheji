#ifndef __SERVO__H__
#define __SERVO__H__

/**
 ******************************************************************************
 * @file    Servo.h
 * @brief   舵机驱动头文件
 * @details 本文件提供了舵机的初始化和角度控制功能
 *          使用PWM信号控制舵机角度，连接在PA8（TIM1_CH1）
 *          PWM频率50Hz，高电平宽度500-2500us对应0-180度
 ******************************************************************************
 */

// ==================== 函数声明 ====================

/**
 * @brief 初始化舵机驱动
 * @note 配置TIM1_CH1（PA8）输出PWM信号
 *       PWM频率50Hz，周期20ms
 */
void Servo_Init(void);

/**
 * @brief 设置舵机角度
 * @param Angle 要设置的角度，范围0~180度
 * @note 角度0度对应500us高电平
 *       角度180度对应2500us高电平
 *       中间角度按线性比例计算
 */
void Servo_SetAngle(float Angle);

// ==================== 宏定义 ====================

/**
 * @brief 舵机开启（180度）
 */
#define Servo_SetON  Servo_SetAngle(180);

/**
 * @brief 舵机关闭（0度）
 */
#define Servo_SetOFF Servo_SetAngle(0);

#endif

