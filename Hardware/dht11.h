#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

// DHT11引脚定义 
#define DHT11_GPIO_PORT     GPIOA
#define DHT11_GPIO_PIN      GPIO_Pin_0
#define DHT11_GPIO_CLK      RCC_APB2Periph_GPIOA

// DHT11引脚操作宏
#define DHT11_DQ_OUT_HIGH() GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)
#define DHT11_DQ_OUT_LOW()  GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)
#define DHT11_DQ_IN()       GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

// DHT11数据结构
typedef struct {
    uint8_t humidity_int;     // 湿度整数部分
    uint8_t humidity_dec;     // 湿度小数部分
    uint8_t temperature_int;  // 温度整数部分
    uint8_t temperature_dec;  // 温度小数部分
    uint8_t check_sum;        // 校验和
    uint8_t valid;            // 数据有效标志
} DHT11_Data_t;

// 基本功能函数声明
void DHT11_Init(void);
uint8_t DHT11_Read_Data(DHT11_Data_t *dht11_data);
void DHT11_Reset(void);
uint8_t DHT11_Check(void);
uint8_t DHT11_Read_Bit(void);
uint8_t DHT11_Read_Byte(void);

// 测试功能函数声明
void DHT11_IO_IN(void);
void DHT11_IO_OUT(void);


#endif
