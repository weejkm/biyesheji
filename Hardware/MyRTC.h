#ifndef __MYRTC_H
#define __MYRTC_H

/**
 ******************************************************************************
 * @file    MyRTC.h
 * @brief   RTC实时时钟驱动头文件
 * @details 本文件提供了RTC的初始化、时间设置、时间读取和字符串解析功能
 *          使用32.768kHz外部低速晶振（LSE）作为RTC时钟源
 *          支持通过字符串格式设置时间，格式为"TIME:YYYY-MM-DD HH:MM:SS"
 ******************************************************************************
 */

// ==================== 全局变量声明 ====================

/**
 * @brief 全局时间数组
 * @note 数组元素依次为：[0]年份、[1]月份、[2]日期、[3]小时、[4]分钟、[5]秒
 *       例如：{2025, 8, 22, 16, 40, 20} 表示 2025年8月22日 16:40:20
 */
extern uint16_t MyRTC_Time[];

// ==================== 函数声明 ====================

/**
 * @brief 初始化RTC
 * @note 首次运行时会配置RTC并设置时间，后续运行会直接使用RTC的时间
 *       使用BKP备份数据寄存器判断是否为首次运行
 */
void MyRTC_Init(void);

/**
 * @brief 设置RTC时间
 * @note 调用此函数会将全局数组MyRTC_Time的值刷新到RTC硬件寄存器
 */
void MyRTC_SetTime(void);

/**
 * @brief 读取RTC时间
 * @note 调用此函数会将RTC硬件寄存器的时间值刷新到全局数组MyRTC_Time
 */
void MyRTC_ReadTime(void);

/**
 * @brief 解析时间字符串并设置RTC
 * @param buf 时间字符串，格式为"TIME:YYYY-MM-DD HH:MM:SS"
 * @return 0-成功, -1-失败
 * @note 字符串可以包含前导或尾随的空白字符、冒号等
 *       例如：":TIME:2025-08-22 16:40:20\r\n" 也可正确解析
 */
int parse_time_and_set_rtc(const char *buf);

#endif

