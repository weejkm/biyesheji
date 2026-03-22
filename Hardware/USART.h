#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"
#include "stdio.h"
#include <stdarg.h>

#define RXBUFFERSIZE 128   // 接收缓存大小

// USART1相关函数
void USART1_Init(uint32_t baudrate);
void USART1_SendByte(uint8_t data);
void USART1_SendString(char *str);
void Debug_Print(char* str);

void USART2_Init(uint32_t baudrate);
void USART2_SendByte(uint8_t data);
void USART2_SendString(char *str);

void USART3_Init(uint32_t baudrate);
void USART3_SendByte(uint8_t data);
void USART3_SendString(char *str);

// 接收缓冲
extern volatile uint8_t USART1_RX_BUF[];
extern volatile uint16_t USART1_RX_CNT;

extern volatile uint8_t USART2_RX_BUF[];
extern volatile uint16_t USART2_RX_CNT;

extern volatile uint8_t USART3_RX_BUF[];
extern volatile uint16_t USART3_RX_CNT;

#endif
