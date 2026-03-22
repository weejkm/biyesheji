#ifndef __USART_H
#define __USART_H

/**
 ******************************************************************************
 * @file    USART.h
 * @brief   串口通信驱动头文件
 * @details 本文件包含了USART1、USART2、USART3的初始化和数据传输函数声明
 *          支持异步串口通信，用于调试、模块通信等场景
 ******************************************************************************
 */

#include "stm32f10x.h"
#include "stdio.h"
#include <stdarg.h>

/**
 * @brief 接收缓冲区大小定义
 * @note 每个串口使用128字节的接收缓冲区
 */
#define RXBUFFERSIZE 128

// ==================== USART1 函数声明 ====================
/**
 * @brief 初始化USART1串口
 * @param baudrate 波特率设置，如115200、9600等
 * @note PA9-TX, PA10-RX
 */
void USART1_Init(uint32_t baudrate);

/**
 * @brief 通过USART1发送一个字节数据
 * @param data 要发送的字节数据
 */
void USART1_SendByte(uint8_t data);

/**
 * @brief 通过USART1发送字符串
 * @param str 要发送的字符串指针
 */
void USART1_SendString(char *str);

/**
 * @brief 调试信息打印函数
 * @param str 要打印的调试信息字符串
 * @note 这是一个封装函数，便于调试输出
 */
void Debug_Print(char* str);

// ==================== USART2 函数声明 ====================
/**
 * @brief 初始化USART2串口
 * @param baudrate 波特率设置
 * @note PA2-TX, PA3-RX
 */
void USART2_Init(uint32_t baudrate);

/**
 * @brief 通过USART2发送一个字节数据
 * @param data 要发送的字节数据
 */
void USART2_SendByte(uint8_t data);

/**
 * @brief 通过USART2发送字符串
 * @param str 要发送的字符串指针
 */
void USART2_SendString(char *str);

// ==================== USART3 函数声明 ====================
/**
 * @brief 初始化USART3串口
 * @param baudrate 波特率设置
 * @note PB10-TX, PB11-RX
 */
void USART3_Init(uint32_t baudrate);

/**
 * @brief 通过USART3发送一个字节数据
 * @param data 要发送的字节数据
 */
void USART3_SendByte(uint8_t data);

/**
 * @brief 通过USART3发送字符串
 * @param str 要发送的字符串指针
 */
void USART3_SendString(char *str);

// ==================== 接收缓冲区声明 ====================
/**
 * @brief USART1 接收缓冲区数组
 * @note 中断接收模式，最大容量128字节
 */
extern volatile uint8_t USART1_RX_BUF[];

/**
 * @brief USART1 接收计数器
 * @note 记录当前接收到的字节数
 */
extern volatile uint16_t USART1_RX_CNT;

/**
 * @brief USART2 接收缓冲区数组
 * @note 中断接收模式，最大容量128字节
 */
extern volatile uint8_t USART2_RX_BUF[];

/**
 * @brief USART2 接收计数器
 * @note 记录当前接收到的字节数
 */
extern volatile uint16_t USART2_RX_CNT;

/**
 * @brief USART3 接收缓冲区数组
 * @note 中断接收模式，最大容量128字节
 */
extern volatile uint8_t USART3_RX_BUF[];

/**
 * @brief USART3 接收计数器
 * @note 记录当前接收到的字节数
 */
extern volatile uint16_t USART3_RX_CNT;

#endif
