#include "stm32f10x.h"                  // Device header
#include "Encoder.h"

int16_t Encoder_Count = 0;              // 全局变量，用于计数旋转编码器的增量值

/**
  * @brief  简单延时函数（约几微秒级）
  * @param  n 循环次数（数值越大延时越长）
  */
static void Encoder_Delay(volatile uint16_t n)
{
    while(n--);
}

/**
  * @brief  旋转编码器初始化
  */
void Encoder_Init(void)
{
    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* GPIO 初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* AFIO 映射 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);

    /* EXTI 初始化 */
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling; // 双沿触发
    EXTI_Init(&EXTI_InitStructure);

    /* NVIC 分组与优先级配置 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_Init(&NVIC_InitStructure);
}

/**
  * @brief  获取旋转编码器增量值（读取后清零）
  */
int16_t Encoder_Get(void)
{
    int16_t Temp = Encoder_Count;
    Encoder_Count = 0;
    return Temp;
}

/**
  * @brief  EXTI0（PB0）中断服务函数
  */
void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) == SET)
    {
        // 【关键修改】增加消抖延时
        // 延时1-2毫秒（具体数值可能需要根据你的编码器调整）
        // 这里的Encoder_Delay(200)可能太短了，我们换一个更长的
        // 你可以使用SysTick或者一个简单的for循环
        Encoder_Delay(1000); // 延时约1ms，屏蔽抖动

        // 重新读取稳定后的电平
        uint8_t A = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);
        uint8_t B = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);

        // 【关键修改】只判断一个边沿，简化逻辑
        // 我们只关心A相的下降沿
        if (A == 0) 
        {
            if (B == 1)
                Encoder_Count++;  // A下降沿，B为高，正转
            else
                Encoder_Count--;  // A下降沿，B为低，反转
        }

        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

/**
  * @brief  EXTI1（PB1）中断服务函数
  */
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) == SET)
    {
        // 【关键修改】增加消抖延时
        Encoder_Delay(1000); // 延时约1ms

        uint8_t A = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);
        uint8_t B = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);

        // 【关键修改】只判断一个边沿
        // 我们只关心B相的下降沿
        if (B == 0) 
        {
            if (A == 0)
                Encoder_Count++;  // B下降沿，A为低，正转
            else
                Encoder_Count--;  // B下降沿，A为高，反转
        }

        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}
