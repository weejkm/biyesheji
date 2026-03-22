/**
 ******************************************************************************
 * @file    Encoder.c
 * @brief   旋转编码器驱动实现文件
 * @details 使用外部中断检测编码器A、B相的边沿变化
 *          通过A、B相的相位关系判断旋转方向
 *          配置了软件消抖时间，提高检测稳定性
 ******************************************************************************
 */

#include "stm32f10x.h"
#include "Encoder.h"

// ==================== 全局变量定义 ====================

/**
 * @brief 全局变量计数，用于计旋转编码器旋转的脉冲数
 * @note 正转时增加，反转时减少
 *       调用Encoder_Get函数会返回当前值并清零
 */
int16_t Encoder_Count = 0;

// ==================== 内部函数 ====================

/**
 * @brief 简单延时函数（微秒级）
 * @param n 循环次数，数值越大延时越长
 * @note 用于软件消抖
 */
static void Encoder_Delay(volatile uint16_t n)
{
    while(n--);
}

// ==================== 编码器初始化函数 ====================

/**
 * @brief 旋转编码器初始化
 * @details 配置步骤：
 *          1. 开启GPIOB和AFIO时钟
 *          2. 配置PB0和PB1为上拉输入模式
 *          3. 配置AFIO映射，将PB0、PB1连接到EXTI0、EXTI1
 *          4. 配置EXTI为上升沿和下降沿触发中断
 *          5. 配置NVIC中断优先级（抢占优先级5，响应优先级1和2）
 */
void Encoder_Init(void)
{
    // 1. 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // 2. GPIO 初始化：配置PB0和PB1为上拉输入
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;        // 上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;  // PB0和PB1
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 3. AFIO 映射：将GPIO引脚连接到外部中断线
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);  // PB0 -> EXTI0
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);  // PB1 -> EXTI1

    // 4. EXTI 初始化：配置为上升沿和下降沿触发
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = EXTI_Line0 | EXTI_Line1;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  // 双边沿触发
    EXTI_Init(&EXTI_InitStructure);

    // 5. NVIC 配置中断优先级分组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // 配置EXTI0中断
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_Init(&NVIC_InitStructure);

    // 配置EXTI1中断
    NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 5;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_Init(&NVIC_InitStructure);
}

// ==================== 编码器计数值读取函数 ====================

/**
 * @brief 获取旋转编码器的计数值（读取后清零）
 * @return 当前计数值
 * @note 读取后会自动清零计数器
 *       适合用于读取旋转增量
 */
int16_t Encoder_Get(void)
{
    int16_t Temp = Encoder_Count;
    Encoder_Count = 0;  // 清零计数器
    return Temp;
}

// ==================== EXTI0中断服务函数 (PB0-A相) ====================

/**
 * @brief EXTI0（PB0-A相）中断服务函数
 * @note 检测A相的边沿变化，通过A、B相的相位关系判断旋转方向
 *      只在A相下降沿时判断：
 *      - A=0, B=1: 正转（计数+1）
 *      - A=0, B=0: 反转（计数-1）
 */
void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line0) == SET)
    {
        // 1. 软件消抖：延时约1ms
        Encoder_Delay(1000);

        // 2. 再次读取稳定的电平
        uint8_t A = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);
        uint8_t B = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);

        // 3. 只判断A相为下降沿的情况
        if (A == 0)
        {
            if (B == 1)
                Encoder_Count++;  // A下降沿，B为高，正转
            else
                Encoder_Count--;  // A下降沿，B为低，反转
        }

        // 4. 清除中断标志
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}

// ==================== EXTI1中断服务函数 (PB1-B相) ====================

/**
 * @brief EXTI1（PB1-B相）中断服务函数
 * @note 检测B相的边沿变化，通过A、B相的相位关系判断旋转方向
 *      只在B相下降沿时判断：
 *      - B=0, A=0: 正转（计数+1）
 *      - B=0, A=1: 反转（计数-1）
 */
void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line1) == SET)
    {
        // 1. 软件消抖：延时约1ms
        Encoder_Delay(1000);

        // 2. 读取电平
        uint8_t A = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_0);
        uint8_t B = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1);

        // 3. 只判断B相为下降沿的情况
        if (B == 0)
        {
            if (A == 0)
                Encoder_Count++;  // B下降沿，A为低，正转
            else
                Encoder_Count--;  // B下降沿，A为高，反转
        }

        // 4. 清除中断标志
        EXTI_ClearITPendingBit(EXTI_Line1);
    }
}
