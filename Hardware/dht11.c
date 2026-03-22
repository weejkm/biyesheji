/**
 ******************************************************************************
 * @file    dht11.c
 * @brief   DHT11温湿度传感器驱动实现文件
 * @details 使用单总线协议读取DHT11温湿度数据
 *          数据格式：湿度整数(8bit) + 湿度小数(8bit) +
 *                   温度整数(8bit) + 温度小数(8bit) + 校验和(8bit)
 ******************************************************************************
 */

#include "dht11.h"
#include <stdio.h>
#include "delay.h"

// ==================== DHT11初始化函数 ====================

/**
 * @brief 初始化DHT11传感器
 * @note 配置PA0为推挽输出模式，默认输出高电平（释放总线）
 *       启动后需要延时1s等待传感器稳定
 */
void DHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 使能GPIO时钟
    RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);

    // 2. 配置引脚为推挽输出模式
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;   // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // GPIO速度50MHz
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);

    // 3. 拉高电平释放总线，让DHT11处于空闲状态
    DHT11_DQ_OUT_HIGH();
    Delay_ms(1000);  // 延时1s等待传感器稳定
}

// ==================== GPIO模式配置函数 ====================

/**
 * @brief 配置DHT11数据引脚为输入模式（内部上拉）
 * @note 主机接收传感器数据时使用此模式
 *       上拉输入模式可以防止引脚悬空
 */
void DHT11_IO_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;   // 上拉输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief 配置DHT11数据引脚为输出模式（推挽）
 * @note 主机发送控制信号时使用此模式
 *       推挽输出可以提供更强的驱动能力
 */
void DHT11_IO_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 强推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

// ==================== DHT11通信控制函数 ====================

/**
 * @brief DHT11复位（发送起始信号）
 * @note 主机拉低18ms，然后拉高40us
 *       DHT11检测到起始信号后会准备发送数据
 */
void DHT11_Reset(void)
{
    DHT11_IO_OUT();           // 切换为输出模式
    DHT11_DQ_OUT_LOW();       // 拉低数据线
    Delay_ms(18);             // 延时18ms（至少18ms的起始信号）
    DHT11_DQ_OUT_HIGH();      // 释放数据线（拉高）
    Delay_us(40);             // 延时40us，等待DHT11准备响应
}

/**
 * @brief 检测DHT11响应信号
 * @return 0-成功（检测到响应）, 1-失败（无响应）
 * @note 正常响应序列：
 *       - DHT11拉低约80us（表示存在）
 *       - DHT11拉高约80us（表示准备发送数据）
 */
uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;

    DHT11_IO_IN();  // 切换为输入模式

    // 1. 等待DHT11拉低数据线（响应信号，约80us）
    while (DHT11_DQ_IN() && retry < 100) {
        retry++;
        Delay_us(1);
    }

    if(retry >= 100) return 1;  // 超时未响应
    else retry = 0;

    // 2. 等待DHT11拉高数据线（准备发送数据，约80us）
    while (!DHT11_DQ_IN() && retry < 100) {
        retry++;
        Delay_us(1);
    }

    if(retry >= 100) {
        //printf("dht11 outtime\r\n");  // 可选：打印超时信息
        return 1;  // 超时
    }

    return 0;  // 响应成功
}

// ==================== 数据位读取函数 ====================

/**
 * @brief 从DHT11读取一位数据
 * @return 读取的位值：0或1
 * @note 根据高电平持续时间判断位值：
 *       - 0：高电平持续约26-28us
 *       - 1：高电平持续约70us
 *       通过延时40us后检测电平状态来区分0和1
 */
uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;

    // 1. 等待线为低电平（每位开始前的50us低电平信号）
    while(DHT11_DQ_IN() && retry < 100) {
        retry++;
        Delay_us(1);
    }

    retry = 0;
    // 2. 等待线为高电平（数据位开始）
    while(!DHT11_DQ_IN() && retry < 100) {
        retry++;
        Delay_us(1);
    }

    Delay_us(40);  // 3. 延时40us判断

    // 4. 检测电平状态
    if(DHT11_DQ_IN())
        return 1;  // 高电平表示数据位1（持续时间长）
    else
        return 0;  // 低电平表示数据位0（持续时间短）
}

/**
 * @brief 从DHT11读取一个字节数据
 * @return 读取的字节值
 * @note 调用8次Read_Bit函数，从高位到低位合并成一个字节
 */
uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat;
    dat = 0;

    for (i = 0; i < 8; i++) {
        dat <<= 1;              // 左移一位
        dat |= DHT11_Read_Bit(); // 读取一位并合并
    }

    return dat;
}

// ==================== 数据读取函数 ====================

/**
 * @brief 读取DHT11温湿度数据
 * @param dht11_data 存储读取数据的结构体指针
 * @return 0-成功, 1-失败
 * @note 读取步骤：
 *       1. 发送起始信号（Reset）
 *       2. 等待并检测响应信号（Check）
 *       3. 读取5个字节数据（40bit）
 *       4. 校验数据有效性
 *       5. 将数据存入结构体
 */
uint8_t DHT11_Read_Data(DHT11_Data_t *dht11_data)
{
    uint8_t buf[5];  // 缓冲区存储40位数据（5个字节）
    uint8_t i;

    // 1. 发送起始信号
    DHT11_Reset();

    // 2. 等待响应
    if(DHT11_Check() == 0) {
        // 3. 读取5个字节数据
        for(i = 0; i < 5; i++) {
            buf[i] = DHT11_Read_Byte();
        }

        // 任务调度器恢复（如果有任务被挂起）
        xTaskResumeAll();

        // 4. 校验和判断：前4个字节之和应该等于第5个字节
        if((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4]) {
            // 5. 数据有效，存入结构体
            dht11_data->humidity_int = buf[0];      // 湿度整数部分
            dht11_data->humidity_dec = buf[1];      // 湿度小数部分
            dht11_data->temperature_int = buf[2];   // 温度整数部分
            dht11_data->temperature_dec = buf[3];   // 温度小数部分
            dht11_data->check_sum = buf[4];         // 校验和
            dht11_data->valid = 1;                  // 标记数据有效
            return 0;  // 成功
        }
    }

    // 数据无效
    dht11_data->valid = 0;  // 标记数据无效
    return 1;               // 失败
}

