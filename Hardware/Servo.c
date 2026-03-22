/**
 ******************************************************************************
 * @file    Servo.c
 * @brief   舵机驱动实现文件
 * @details 使用TIM1的通道1（PA8）输出PWM信号控制舵机角度
 *          PWM频率50Hz（周期20ms），高电平宽度500-2500us对应0-180度
 ******************************************************************************
 */

#include "Servo.h"
#include "stm32f10x.h"

// ==================== PWM初始化函数 ====================

/**
 * @brief PWM初始化
 * @details 配置TIM1_CH1（PA8）输出PWM信号
 *          PWM频率：72MHz/72/20000 = 50Hz（周期20ms）
 *          占空比范围：0~2000（对应0~100%）
 *          舵机控制：500us（2.5%）对应0度，2500us（12.5%）对应180度
 */
void PWM_Init(void)
{
    // 1. 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);  // TIM1在APB2总线上
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 2. GPIO初始化：配置PA8为复用推挽输出
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;        // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;              // PA8引脚（TIM1_CH1）
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. 时基单元初始化
    TIM_InternalClockConfig(TIM1);  // 选择TIM1为内部时钟

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;      // 时钟分频，不分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
    TIM_TimeBaseInitStructure.TIM_Period = 20000 - 1;               // 周期ARR=19999
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;               // 预分频PSC=71
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;            // 重复计数器（高级定时器）
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

    // PWM频率计算：72MHz / (71+1) / (19999+1) = 50Hz（周期20ms）

    // 4. 输出比较初始化
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);  // 结构体初始化，赋默认值
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;              // PWM模式1
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;      // 输出极性为高
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  // 输出使能
    TIM_OCInitStructure.TIM_Pulse = 0;                             // 初始CCR=0
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);  // 配置TIM1输出比较通道1

    // 5. 关键修改点：TIM1主输出使能
    // TIM1是高级定时器，输出PWM必须使能主输出
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    // 6. TIM使能
    TIM_Cmd(TIM1, ENABLE);  // 使能TIM1，定时器开始运行
}

// ==================== PWM占空比设置函数 ====================

/**
 * @brief PWM设置CCR值
 * @param Compare 要写入CCR的值，范围0~19999（对应ARR=20000-1）
 * @note CCR和ARR共同决定占空比：Duty = CCR / (ARR + 1)
 *       舵机控制：
 *       - 0度：CCR=500（占空比2.5%，高电平500us）
 *       - 180度：CCR=2500（占空比12.5%，高电平2500us）
 */
void PWM_SetCompare2(uint16_t Compare)
{
    TIM_SetCompare1(TIM1, Compare);  // 设置TIM1_CH1的CCR1值
}

// ==================== 舵机初始化函数 ====================

/**
 * @brief 舵机初始化
 * @note 调用PWM底层初始化函数
 */
void Servo_Init(void)
{
    PWM_Init();  // 初始化舵机的底层PWM
}

// ==================== 舵机角度设置函数 ====================

/**
 * @brief 舵机设置角度
 * @param Angle 要设置的角度，范围：0~180度
 * @note 将角度映射到CCR值：
 *       0度 -> CCR=500（高电平500us）
 *       180度 -> CCR=2500（高电平2500us）
 *       计算公式：CCR = Angle/180 * 2000 + 500
 */
void Servo_SetAngle(float Angle)
{
    // 设置占空比
    // 将角度线性映射到CCR值范围500~2500
    PWM_SetCompare2(Angle / 180 * 2000 + 500);
}

