/**
 ******************************************************************************
 * @file    light.c
 * @brief   光照传感器驱动实现文件
 * @details 使用ADC1的通道1（PA1）读取光敏电阻的电压值来检测光照强度
 *          ADC为12位分辨率，转换值范围0~4095
 ******************************************************************************
 */

#include "stm32f10x.h"
#include "light.h"
#include "stdio.h"

// ==================== 光照传感器引脚定义 ====================

/**
 * @brief 光照传感器引脚定义
 * @note 连接在PA1引脚，使用ADC1的通道1
 */
#define LIGHT_GPIO GPIOA           // GPIOA端口
#define LIGHT_GPIO_PIN GPIO_Pin_1  // PA1引脚
#define LIGHT_GPIO_CLK RCC_APB2Periph_GPIOA  // GPIOA时钟

// ==================== GPIO初始化函数 ====================

/**
 * @brief 初始化光照传感器的GPIO
 * @details 配置PA1引脚为模拟输入模式
 * @note 模拟输入模式不使用施密特触发器，减少功耗
 */
void Init_GPIO(void){
    // 1. 使能GPIOA时钟
    RCC_APB2PeriphClockCmd(LIGHT_GPIO_CLK, ENABLE);

    // 2. 配置PA1为模拟输入模式
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Pin = LIGHT_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;  // 模拟输入模式
    GPIO_Init(LIGHT_GPIO, &GPIO_InitStruct);
}

// ==================== ADC初始化函数 ====================

/**
 * @brief 初始化ADC1
 * @details 配置步骤：
 *          1. 初始化GPIO（配置PA1为模拟输入）
 *          2. 配置ADC时钟（PCLK2的6分频，即12MHz）
 *          3. 配置ADC参数（独立模式、单次转换、右对齐等）
 *          4. 配置ADC通道1（PA1）、采样时间为55.5周期
 *          5. 使能ADC并进行校准
 */
void Init_ADC1(void){
    // 1. 先初始化GPIO
    Init_GPIO();

    // 2. 配置ADC时钟：PCLK2(72MHz)/6 = 12MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    // 3. 使能ADC1时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    // 4. 配置ADC1参数
    ADC_InitTypeDef ADC_InitStruct = {0};
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;              // 独立模式
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;             // 单次转换模式
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;                   // 单通道转换模式
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 软件触发
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;          // 数据右对齐
    ADC_InitStruct.ADC_NbrOfChannel = 1;                         // 转换通道数为1
    ADC_Init(ADC1, &ADC_InitStruct);

    // 5. 配置规则通道：通道1（PA1），采样时间55.5个周期
    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);

    // 6. 使能ADC1
    ADC_Cmd(ADC1, ENABLE);

    // 7. ADC校准过程
    ADC_ResetCalibration(ADC1);                      // 复位校准寄存器
    while(ADC_GetResetCalibrationStatus(ADC1));      // 等待复位完成

    ADC_StartCalibration(ADC1);                      // 开始校准
    while(ADC_GetCalibrationStatus(ADC1));           // 等待校准完成
}

// ==================== ADC读取函数 ====================

/**
 * @brief 读取光照传感器的ADC值
 * @return ADC转换值，范围0~4095
 * @note 该函数为阻塞式函数，会等待ADC转换完成
 *       返回值越大表示光照越强（电压越高）
 *       ADC分辨率：12位，0~4095对应0~3.3V
 */
uint16_t Read_ADC_Value(void) {
    uint16_t light_value = 0;

    // 启动ADC转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);  // 启动一次软件触发转换

    // 等待转换完成
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);  // 等待转换结束标志

    // 读取转换结果
    light_value = ADC_GetConversionValue(ADC1);

    return light_value;
}

