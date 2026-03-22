#ifndef __LIGHT_H__
#define __LIGHT_H__

/**
 ******************************************************************************
 * @file    light.h
 * @brief   光照传感器驱动头文件
 * @details 本文件提供了基于ADC的光照传感器初始化和数值读取函数
 *          使用ADC1的通道1（PA1）读取光敏电阻的电压值来检测光照强度
 ******************************************************************************
 */

#include "stm32f10x.h"

// ==================== 光照传感器引脚定义 ====================

/**
 * @brief 光照传感器引脚定义
 * @note 连接在PA1引脚，使用ADC1的通道1
 */
#define LIGHT_GPIO GPIOA           // GPIOA端口
#define LIGHT_GPIO_PIN GPIO_Pin_1  // PA1引脚
#define LIGHT_GPIO_CLK RCC_APB2Periph_GPIOA  // GPIOA时钟

// ==================== 函数声明 ====================

/**
 * @brief 初始化光照传感器的GPIO
 * @note 配置PA1为模拟输入模式
 */
void Init_GPIO(void);

/**
 * @brief 初始化ADC1
 * @details 配置步骤：
 *          1. 初始化GPIO（配置PA1为模拟输入）
 *          2. 配置ADC时钟（PCLK2的6分频，即12MHz）
 *          3. 配置ADC参数（独立模式、单次转换、右对齐等）
 *          4. 配置ADC通道1（PA1）、采样时间为55.5周期
 *          5. 使能ADC并进行校准
 */
void Init_ADC1(void);

/**
 * @brief 读取光照传感器的ADC值
 * @return ADC转换值，范围0~4095
 * @note 该函数为阻塞式函数，会等待ADC转换完成
 *       返回值越大表示光照越强（电压越高）
 */
uint16_t Read_ADC_Value(void);

#endif
