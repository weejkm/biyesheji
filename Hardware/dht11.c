#include "dht11.h"
#include <stdio.h>
#include "delay.h"


/**
 * @brief DHT11初始化
 */
void DHT11_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIO时钟
    RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);
    
    // 初始配置为推挽输出
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
    
    // 保持高电平，让DHT11稳定
    DHT11_DQ_OUT_HIGH();
    Delay_ms(1000);
	
}

/**
 * @brief 配置DHT11引脚为输入模式（带内部上拉）
 */
void DHT11_IO_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 带内部上拉的输入
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief 配置DHT11引脚为输出模式（推挽输出）
 */
void DHT11_IO_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    GPIO_InitStructure.GPIO_Pin = DHT11_GPIO_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  // 推挽输出，更强的驱动能力
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}


/**
 * @brief DHT11复位
 */
void DHT11_Reset(void)
{
    DHT11_IO_OUT();
    DHT11_DQ_OUT_LOW();     // 拉低数据线
    Delay_ms(18);     // 延时18ms（标准时序）
    DHT11_DQ_OUT_HIGH();    // 拉高数据线
    Delay_us(40);     // 延时40us，给DHT11更多反应时间
}

/**
 * @brief 等待DHT11响应
 * @return 0-成功, 1-失败
 */
uint8_t DHT11_Check(void)
{
    uint8_t retry = 0;
    
    DHT11_IO_IN(); // 设置为输入
    
    // 等待DHT11拉低数据线（响应信号）
    while (DHT11_DQ_IN() && retry < 100) {
        retry++;
        Delay_us(1);
    }
    
    if(retry >= 100) return 1; // 超时，无响应
    else retry = 0;
    
    // 等待DHT11拉高数据线（准备发送数据）
    while (!DHT11_DQ_IN() && retry < 100) {
        retry++;
        Delay_us(1);
    }
    
    if(retry >= 100) {
		//printf("dht11 outtime\r\n");
		return 1; // 超时
	}
	
    
    return 0;
}

/**
 * @brief 从DHT11读取一个位
 * @return 读取到的位值
 */
uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;
    
    // 等待变为低电平（每位数据前的低电平信号）
    while(DHT11_DQ_IN() && retry < 100) {
        retry++;
        Delay_us(1);
    }
    
    retry = 0;
    // 等待变为高电平（数据位）
    while(!DHT11_DQ_IN() && retry < 100) {
        retry++;
        Delay_us(1);
    }
    
    Delay_us(40); // 延时40us后判断
    
    if(DHT11_DQ_IN()) return 1; // 高电平表示数据位1
    else return 0;              // 低电平表示数据位0
}

/**
 * @brief 从DHT11读取一个字节
 * @return 读取到的字节
 */
uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat;
    dat = 0;
    
    for (i = 0; i < 8; i++) {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    
    return dat;
}

/**
 * @brief 读取DHT11数据
 * @param dht11_data: 存储读取数据的结构体指针
 * @return 0-成功, 1-失败
 */
uint8_t DHT11_Read_Data(DHT11_Data_t *dht11_data)
{
    uint8_t buf[5];
    uint8_t i;
    
    DHT11_Reset();
    
    if(DHT11_Check() == 0) {
        for(i = 0; i < 5; i++) {
            buf[i] = DHT11_Read_Byte();
        }
        
        // 恢复任务调度
        xTaskResumeAll();
        
        // 校验数据
        if((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4]) {
            dht11_data->humidity_int = buf[0];
            dht11_data->humidity_dec = buf[1];
            dht11_data->temperature_int = buf[2];
            dht11_data->temperature_dec = buf[3];
            dht11_data->check_sum = buf[4];
            dht11_data->valid = 1;
            return 0;
        }
    }
    
    
    dht11_data->valid = 0;
    return 1;
}
