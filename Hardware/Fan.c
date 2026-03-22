/**
 ******************************************************************************
 * @file    Fan.c
 * @brief   直流电机（风扇）驱动实现文件
 * @details 通过GPIO控制风扇的开关状态
 *          风扇连接在PB8引脚，低电平开启，高电平关闭
 ******************************************************************************
 */

#include "Fan.h"

// ==================== 风扇初始化函数 ====================

/**
 * @brief 初始化风扇驱动
 * @note 配置PB8为推挽输出模式，默认输出高电平（风扇关闭）
 */
void Fan_Init(void)
{
    // 1. 使能GPIO时钟
    RCC_APB2PeriphClockCmd(Fan_GPIO_CLK, ENABLE); // 使能GPIOB时钟

    // 2. 配置GPIO为推挽输出模式
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Pin = Fan_GPIO_PIN;      // PB8引脚
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出模式
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; // GPIO速度50MHz
    GPIO_Init(Fan_GPIO_PORT, &GPIO_InitStruct);

    // 3. 默认设置为高电平（风扇关闭）
    GPIO_SetBits(Fan_GPIO_PORT, Fan_GPIO_PIN);
}

// ==================== 风扇控制函数 ====================

/**
 * @brief 设置风扇状态（开/关）
 * @param STAT 风扇状态：0-关闭，1-开启
 * @note 0: 输出高电平，风扇关闭
 *       1: 输出低电平，风扇开启（低电平有效）
 */
void Fan_set(uint8_t STAT){
    if(STAT)
        GPIO_ResetBits(Fan_GPIO_PORT, Fan_GPIO_PIN);  // 低电平，风扇开启
    else
        GPIO_SetBits(Fan_GPIO_PORT, Fan_GPIO_PIN);    // 高电平，风扇关闭
}
