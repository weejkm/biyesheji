#include "delay.h"
#include "stm32f10x.h"                  // Device header


/**
 * @brief 初始化用于延时的定时器
 */
void Timer_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 配置定时器为1MHz（1us精度）
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = (SystemCoreClock / 1000000) - 1; // 72-1
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM2, ENABLE);
}

void delay_init(void){
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 配置定时器为1MHz（1us精度）
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;
    TIM_TimeBaseStructure.TIM_Prescaler = (SystemCoreClock / 1000000) - 1; // 72-1
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    TIM_Cmd(TIM2, ENABLE);
}

/**
 * @brief 定时器版微秒延时
 * @param us: 延时微秒数
 */
void Delay_us(uint32_t us)
{
    uint16_t start = TIM_GetCounter(TIM2);
    
    while((uint16_t)(TIM_GetCounter(TIM2) - start) < us);
}
void delay_us(uint32_t us)
{
    uint16_t start = TIM_GetCounter(TIM2);
    
    while((uint16_t)(TIM_GetCounter(TIM2) - start) < us);
}


/**
 * @brief 毫秒延时函数
 * @param ms: 延时毫秒数
 */
void Delay_ms(uint32_t ms)
{
    for(uint32_t i = 0; i < ms; i++) {
        Delay_us(1000);
    }
}

void delay_ms(uint32_t ms)
{
    for(uint32_t i = 0; i < ms; i++) {
        Delay_us(1000);
    }
}
