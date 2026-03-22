#ifndef __DHT11_H
#define __DHT11_H

/**
 ******************************************************************************
 * @file    dht11.h
 * @brief   DHT11温湿度传感器驱动头文件
 * @details 本文件提供了DHT11温湿度传感器的初始化、数据读取等功能
 *          使用单总线协议通信，温度范围0-50℃，湿度范围20-90%RH
 *          传感器连接在PA0引脚
 ******************************************************************************
 */

#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"

// ==================== DHT11引脚定义 ====================

/**
 * @brief DHT11温湿度传感器引脚定义
 * @note 传感器数据引脚连接在PA0
 */
#define DHT11_GPIO_PORT     GPIOA               // GPIO端口
#define DHT11_GPIO_PIN      GPIO_Pin_0          // GPIO引脚PA0
#define DHT11_GPIO_CLK      RCC_APB2Periph_GPIOA  // GPIO时钟

// ==================== DHT11引脚操作宏 ====================

/**
 * @brief DHT11数据引脚输出高电平
 * @note 主机释放数据线
 */
#define DHT11_DQ_OUT_HIGH() GPIO_SetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

/**
 * @brief DHT11数据引脚输出低电平
 * @note 主机拉低数据线
 */
#define DHT11_DQ_OUT_LOW()  GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

/**
 * @brief 读取DHT11数据引脚电平
 * @return Bit_RESET（0）或Bit_SET（1）
 */
#define DHT11_DQ_IN()       GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_GPIO_PIN)

// ==================== DHT11数据结构定义 ====================

/**
 * @brief DHT11数据结构体
 * @details 存储40位数据，包括8位湿度整数、8位湿度小数、
 *          8位温度整数、8位温度小数、8位校验和
 */
typedef struct {
    uint8_t humidity_int;     // 湿度整数部分（范围20-90）
    uint8_t humidity_dec;     // 湿度小数部分（通常为0）
    uint8_t temperature_int;  // 温度整数部分（范围0-50）
    uint8_t temperature_dec;  // 温度小数部分（通常为0）
    uint8_t check_sum;        // 校验和（前4个字节之和）
    uint8_t valid;            // 数据有效标志：0-无效，1-有效
} DHT11_Data_t;

// ==================== 功能函数声明 ====================

/**
 * @brief 初始化DHT11传感器
 * @note 配置PA0为推挽输出模式，默认输出高电平
 *       启动后需要延时1ms等待传感器稳定
 */
void DHT11_Init(void);

/**
 * @brief 读取DHT11温湿度数据
 * @param dht11_data 存储读取数据的数据结构体指针
 * @return 0-成功, 1-失败
 * @note 该函数会自动检测校验和，数据错误时返回失败
 */
uint8_t DHT11_Read_Data(DHT11_Data_t *dht11_data);

/**
 * @brief 复位DHT11通信
 * @note 主机发送起始信号：拉低18ms，然后拉高40us
 */
void DHT11_Reset(void);

/**
 * @brief 检测DHT11响应信号
 * @return 0-成功（检测到响应）, 1-失败（无响应）
 * @note DHT11在收到起始信号后会拉低80us，然后拉高80us表示响应
 */
uint8_t DHT11_Check(void);

/**
 * @brief 从DHT11读取一位数据
 * @return 读取的位值：0或1
 * @note 根据高电平持续时间判断位值：
 *       26-28us高电平表示0
 *       70us高电平表示1
 */
uint8_t DHT11_Read_Bit(void);

/**
 * @brief 从DHT11读取一个字节数据
 * @return 读取的字节值
 * @note 调用8次Read_Bit函数，合并成一个字节
 */
uint8_t DHT11_Read_Byte(void);

// ==================== GPIO模式配置函数声明 ====================

/**
 * @brief 配置DHT11数据引脚为输入模式（上拉）
 * @note 用于主机接收传感器数据时
 */
void DHT11_IO_IN(void);

/**
 * @brief 配置DHT11数据引脚为输出模式（推挽）
 * @note 用于主机发送控制信号时
 */
void DHT11_IO_OUT(void);

#endif

