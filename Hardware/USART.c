/**
 ******************************************************************************
 * @file    USART.c
 * @brief   串口通信驱动实现文件
 * @details 本文件包含了USART1、USART2、USART3的初始化和数据传输函数实现
 *          支持波特率配置、中断接收、数据发送等功能
 ******************************************************************************
 */

#include "USART.h"
#include <string.h>
#include <stdio.h>

// ==================== 接收缓冲区定义（中断接收模式） ====================

/**
 * @brief USART1 接收缓冲区
 * @note 存储USART1接收到 的数据，最大128字节
 */
volatile uint8_t USART1_RX_BUF[RXBUFFERSIZE];

/**
 * @brief USART1 接收计数器
 * @note 指示当前接收缓冲区的数据长度
 */
volatile uint16_t USART1_RX_CNT = 0;

/**
 * @brief USART2 接收缓冲区
 * @note 存储USART2接收到的数据，最大128字节
 */
volatile uint8_t USART2_RX_BUF[RXBUFFERSIZE];

/**
 * @brief USART2 接收计数器
 * @note 指示当前接收缓冲区的数据长度
 */
volatile uint16_t USART2_RX_CNT = 0;

/**
 * @brief USART3 接收缓冲区
 * @note 存储USART3接收到的数据，最大128字节
 */
volatile uint8_t USART3_RX_BUF[RXBUFFERSIZE];

/**
 * @brief USART3 接收计数器
 * @note 指示当前接收缓冲区的数据长度
 */
volatile uint16_t USART3_RX_CNT = 0;

// ==================== USART1 初始化和通信函数 ====================

/**
 * @brief 初始化USART1串口
 * @param baudrate 波特率设置（如115200、9600等）
 * @note 硬件连接：PA9-TX, PA10-RX
 *       配置了接收中断，优先级为3,3
 */
void USART1_Init(uint32_t baudrate)
{
    // 1. 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // 2. GPIO 配置
    GPIO_InitTypeDef GPIO_InitStructure;

    // 配置 TX 引脚 (PA9) - 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 RX 引脚 (PA10) - 浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. USART 参数配置
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baudrate;                    // 波特率
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;       // 8位数据位
    USART_InitStructure.USART_StopBits = USART_StopBits_1;            // 1位停止位
    USART_InitStructure.USART_Parity = USART_Parity_No;               // 无校验位
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无硬件流控
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;   // 收发模式
    USART_Init(USART1, &USART_InitStructure);

    // 4. 使能接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    // 5. NVIC 配置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;         // 抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;                // 响应优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 6. 使能USART
    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief 通过USART1发送一个字节
 * @param data 要发送的字节数据
 * @note 阻塞式发送，等待发送缓冲区为空
 */
void USART1_SendByte(uint8_t data)
{
    // 等待发送缓冲区为空
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    // 发送数据
    USART_SendData(USART1, data);
}

/**
 * @brief 通过USART1发送字符串
 * @param str 要发送的字符串指针
 * @note 发送直到遇到字符串结束符'\0'
 */
void USART1_SendString(char *str)
{
    while (*str)
    {
        USART1_SendByte(*str++);
    }
}

/**
 * @brief 调试信息打印函数
 * @param str 要打印的调试信息字符串
 * @note 该函数通过USART1发送调试信息
 */
void Debug_Print(char* str)
{
    USART1_SendString(str);
}

/**
 * @brief 重定向标准输出函数printf
 * @param ch 要输出的字符
 * @param f 文件指针（未使用）
 * @return 返回输出的字符
 * @note 标准库printf函数会调用此函数，实现通过串口输出
 */
int fputc(int ch, FILE *f)
{
    USART1_SendByte(ch);
    return ch;
}

// ==================== USART2 初始化和通信函数 ====================

/**
 * @brief 初始化USART2串口
 * @param baudrate 波特率设置（如115200、9600等）
 * @note 硬件连接：PA2-TX, PA3-RX
 *       配置了接收中断，优先级为3,3
 */
void USART2_Init(uint32_t baudrate)
{
    // 1. 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // 2. GPIO 配置
    GPIO_InitTypeDef GPIO_InitStructure;

    // 配置 TX 引脚 (PA2) - 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 RX 引脚 (PA3) - 浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. USART 参数配置
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    // 4. 使能接收中断
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);

    // 5. NVIC 配置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 6. 使能USART
    USART_Cmd(USART2, ENABLE);
}

/**
 * @brief 通过USART2发送一个字节
 * @param data 要发送的字节数据
 * @note 阻塞式发送，等待发送缓冲区为空
 */
void USART2_SendByte(uint8_t data)
{
    // 等待发送缓冲区为空
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
    // 发送数据
    USART_SendData(USART2, data);
}

/**
 * @brief 通过USART2发送字符串
 * @param str 要发送的字符串指针
 * @note 发送直到遇到字符串结束符'\0'
 */
void USART2_SendString(char *str)
{
    while (*str)
    {
        USART2_SendByte(*str++);
    }
}

// ==================== USART3 初始化和通信函数 ====================

/**
 * @brief 初始化USART3串口
 * @param baudrate 波特率设置（如115200、9600等）
 * @note 硬件连接：PB10-TX, PB11-RX
 *       配置了接收中断，优先级为3,3
 */
void USART3_Init(uint32_t baudrate)
{
    // 1. 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    // 2. GPIO 配置
    GPIO_InitTypeDef GPIO_InitStructure;

    // 配置 TX 引脚 (PB10) - 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 配置 RX 引脚 (PB11) - 浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // 3. USART 参数配置
    USART_InitTypeDef USART_InitStructure;
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);

    // 4. 使能接收中断
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    // 5. NVIC 配置
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // 6. 使能USART
    USART_Cmd(USART3, ENABLE);
}

/**
 * @brief 通过USART3发送一个字节
 * @param data 要发送的字节数据
 * @note 同时发送到USART1，便于调试
 */
void USART3_SendByte(uint8_t data)
{
    // 等待发送缓冲区为空
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
    // 通过USART3发送数据
    USART_SendData(USART3, data);
    // 同时通过USART1发送，便于调试观察
    USART_SendData(USART1, data);
}

/**
 * @brief 通过USART3发送字符串
 * @param str 要发送的字符串指针
 * @note 发送直到遇到字符串结束符'\0'
 */
void USART3_SendString(char *str)
{
    while (*str)
    {
        USART3_SendByte(*str++);
    }
}

// ==================== 中断服务函数 ====================

/**
 * @brief USART1 接收中断服务函数
 * @note 当接收到数据时触发中断，将数据存入接收缓冲区
 *       缓冲区满时丢失新接收的数据
 */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        // 读取接收到的数据
        uint8_t data = USART_ReceiveData(USART1);
        // 存入接收缓冲区，防止溢出
        if (USART1_RX_CNT < RXBUFFERSIZE)
        {
            USART1_RX_BUF[USART1_RX_CNT++] = data;
        }
        // 清除接收中断标志
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

// ==================== 注释代码 ====================
/*
// USART3中断服务函数（已注释，如需要可取消注释）
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        // 读取接收到的数据
        uint8_t data = USART_ReceiveData(USART3);
        // 存入接收缓冲区，防止溢出
        if (USART3_RX_CNT < RXBUFFERSIZE)
        {
            USART3_RX_BUF[USART3_RX_CNT++] = data;
        }
        // 同时通过USART1发送数据，便于调试
        USART1_SendByte(data);
        // 清除接收中断标志
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}
*/
