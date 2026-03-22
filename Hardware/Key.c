#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Key.h"


void Key_Init(void)
{
    
    //RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_5 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}


KeyState Key_GetState(KeyNum key)
{
    KeyState state = NCLICK;
    switch (key)
    {
         case KEY1:
            if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == Bit_RESET){
                 state = CLICK;
                 while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_4) == Bit_RESET);
             }
             break;
         case KEY2:
             if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5) == Bit_RESET){
                 state = CLICK;
                 while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_5) == Bit_RESET);
             }
             break;
        case KEY3:
            if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9) == Bit_RESET){
                state = CLICK;
                while(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9) == Bit_RESET);
            }
            break;

        default:
            break;
    }
    return state;
}




