#include "LED.h"


void LED_Init(void){
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_TIM1,ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStruct={0};
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_11;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure={0};
	TIM_TimeBaseStructure.TIM_Period = 999;         // 自动重装载值 ARR
    TIM_TimeBaseStructure.TIM_Prescaler = 71;       // 预分频器 PSC
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);


	//配置PWM模式 (通道4)
	TIM_OCInitTypeDef TIM_OCInitStructure={0};
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;           // 配置为PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; // 使能输出
    TIM_OCInitStructure.TIM_Pulse = 0;                       // 设置初始占空比 CCR4 (499/999 ≈ 50%)
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;  // 输出极性为高
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);                   // 初始化TIM1通道4

    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable); // 使能CCR4预装载寄存器

    // 5. 启动定时器
    TIM_ARRPreloadConfig(TIM1, ENABLE); // 使能ARR预装载寄存器
    TIM_CtrlPWMOutputs(TIM1, ENABLE);   // 主输出使能，对于高级定时器TIM1和TIM8是必须的
    TIM_Cmd(TIM1, ENABLE);              // 使能TIM1

}

void LED_Set_light(uint8_t light){
	TIM_SetCompare4(TIM1, light*10);
}

