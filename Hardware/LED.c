/**
 ******************************************************************************
 * @file    LED.c
 * @brief   LED控制驱动实现文件
 * @details 使用TIM1的通道4（PA11）输出PWM信号来控制LED亮度
 *          支持0-25级亮度调节
 ******************************************************************************
 */

#include "LED.h"

// ==================== LED初始化函数 ====================

/**
 * @brief 初始化LED驱动
 * @details 配置步骤：
 *          1. 使能GPIOA和TIM1时钟
 *          2. 配置PA11为复用推挽输出（TIM1_CH4）
 *          3. 配置TIM1时基，PWM频率约为1kHz
 *          4. 配置TIM1通道4为PWM模式
 *          5. 使能TIM1预装载和PWM输出
 *          6. 启动TIM1
 */
void LED_Init(void){
    // 1. 使能时钟：GPIOA和TIM1都在APB2总线上
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_TIM1, ENABLE);

    // 2. GPIO配置：PA11配置为复用推挽输出
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;    // 复用推挽输出
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11;        // PA11引脚
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;  // GPIO速度50MHz
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 3. TIM1时基配置
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure = {0};
    TIM_TimeBaseStructure.TIM_Period = 999;               // 自动重装载值 ARR (PWM周期)
    TIM_TimeBaseStructure.TIM_Prescaler = 71;             // 预分频值 PSC (72MHz/(71+1)=1MHz)
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; // 时钟分割
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数模式
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    // PWM频率计算: Fpwm = 72MHz / (71+1) / (999+1) = 1kHz

    // 4. 配置PWM模式（使用通道4）
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;             // 设置为PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 使能输出
    TIM_OCInitStructure.TIM_Pulse = 0;                           // 设置初始占空比 CCR4=0 (关闭)
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;      // 输出极性为低电平有效
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);                     // 初始化TIM1通道4

    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable); // 使能CCR4预装载寄存器

    // 5. 启动定时器
    TIM_ARRPreloadConfig(TIM1, ENABLE);          // 使能ARR预装载寄存器
    TIM_CtrlPWMOutputs(TIM1, ENABLE);            // 主输出使能，针对高级定时器TIM1
    TIM_Cmd(TIM1, ENABLE);                       // 使能TIM1
}

// ==================== LED亮度控制函数 ====================

/**
 * @brief 设置LED亮度
 * @param light 亮度值，范围0-25
 *        - 0: LED关闭
 *        - 1: 最亮 (10/1000 = 1%占空比)
 *        - 25: 最亮 (250/1000 = 25%占空比，注意极性为低电平有效)
 * @note PWM占空比 = light * 10 / 1000
 *       由于输出极性为低电平有效，较小的CCR值对应较高的亮度
 */
void LED_Set_light(uint8_t light){
    // 设置比较寄存器的值，控制PWM占空比
    TIM_SetCompare4(TIM1, light*10);
}

