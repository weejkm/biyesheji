#include "stm32f10x.h"
#include "light.h"
#include "stdio.h"

#define LIGHT_GPIO GPIOA
#define LIGHT_GPIO_PIN GPIO_Pin_1  // 修改为 PA1
#define LIGHT_GPIO_CLK RCC_APB2Periph_GPIOA

void Init_GPIO(void){

    RCC_APB2PeriphClockCmd(LIGHT_GPIO_CLK, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Pin = LIGHT_GPIO_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;  // 设置为模拟输入
    GPIO_Init(LIGHT_GPIO, &GPIO_InitStruct);
}

void Init_ADC1(void){
    Init_GPIO();
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72/6 = 12 MHz，合适

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    ADC_InitTypeDef ADC_InitStruct = {0};
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStruct.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &ADC_InitStruct);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_55Cycles5);  // 修改为 ADC_Channel_1
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);                      // 复位校准寄存器
    while(ADC_GetResetCalibrationStatus(ADC1));      // 等待复位完成

    ADC_StartCalibration(ADC1);                      // 开始校准
    while(ADC_GetCalibrationStatus(ADC1));           // 等待校准完成
}

uint16_t Read_ADC_Value(void) {
    uint16_t light_value = 0;
    // 启动转换
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);  // 启动一次软件触发的转换
    // 等待转换完成
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    // 获取转换结果
    light_value = ADC_GetConversionValue(ADC1);

    return light_value;
}
