#include "Fan.h"

/**
 * @brief  初始化风扇控制引脚和定时器为PWM模式
 * @param  None
 * @retval None
 */
void Fan_Init(void)
{
    // --- 1. 初始化时钟 ---
    // 开启GPIO和TIMx的时钟
    RCC_AHBPeriphClockCmd(Fan_GPIO_CLK, ENABLE);

    // --- 2. 初始化GPIO为复用推挽输出模式 ---
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Pin = Fan_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出，用于TIM PWM
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(Fan_GPIO_PORT, &GPIO_InitStruct);
    
	GPIO_SetBits(Fan_GPIO_PORT,Fan_GPIO_PIN);
}
void Fan_set(uint8_t STAT){
	if(STAT) GPIO_ResetBits(Fan_GPIO_PORT,Fan_GPIO_PIN);
	else GPIO_SetBits(Fan_GPIO_PORT,Fan_GPIO_PIN);
}
