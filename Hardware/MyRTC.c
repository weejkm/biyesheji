/**
 ******************************************************************************
 * @file    MyRTC.c
 * @brief   RTC实时时钟驱动实现文件
 * @details 使用STM32内部RTC模块实现实时时钟功能
 *          时钟源为外部32.768kHz低速晶振（LSE）
 *          支持通过字符串设置时间，格式为"TIME:YYYY-MM-DD HH:MM:SS"
 ******************************************************************************
 */

#include "stm32f10x.h"
#include <time.h>
#include "MyRTC.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// ==================== 全局变量定义 ====================

/**
 * @brief 全局时间数组
 * @note 数组元素依次为：[0]年份、[1]月份、[2]日期、[3]小时、[4]分钟、[5]秒
 *       例如：{2025, 8, 22, 16, 40, 20} 表示 2025年8月22日 16:40:20
 *       该数组用于存储和更新RTC的时间信息
 */
uint16_t MyRTC_Time[] = {2025, 8, 22, 16, 40, 20};

void MyRTC_SetTime(void);  // 函数声明

// ==================== RTC初始化函数 ====================

/**
 * @brief 初始化RTC
 * @note 首次运行时（BKP_DR1 != 0xA5A5）会配置RTC并设置时间
 *       后续运行会直接使用RTC的时间，保持时间连续性
 * @details 初始化步骤：
 *          1. 开启PWR和BKP时钟
 *          2. 使能后备寄存器访问
 *          3. 检查是否首次运行（通过BKP_DR1标志位）
 *          4. 首次运行：
 *             - 开启LSE时钟并等待就绪
 *             - 选择RTC时钟源为LSE
 *             - 配置RTC预分频器（32767，计数频率为1Hz）
 *             - 设置初始时间
 *             - 写入首次运行标志到BKP_DR1
 *          5. 非首次运行：等待RTC同步
 */
void MyRTC_Init(void)
{
    // 1. 开启时钟：PWR电源管理时钟和BKP备份寄存器时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

    // 2. 备份寄存器访问使能
    PWR_BackupAccessCmd(ENABLE);  // 使能PWR和后备寄存器的访问

    // 3. 通过备份寄存器判断是否首次运行
    if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)  // 首次上电
    {
        // --- 首次运行配置RTC ---
        RCC_LSEConfig(RCC_LSE_ON);  // 开启LSE（外部低速晶振32.768kHz）
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != SET);  // 等待LSE稳定

        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);  // 选择RTC时钟源为LSE
        RCC_RTCCLKCmd(ENABLE);                   // 使能RTC时钟

        RTC_WaitForSynchro();     // 等待RTC寄存器同步
        RTC_WaitForLastTask();    // 等待最后一次写操作完成

        RTC_SetPrescaler(32768 - 1);  // 设置预分频器（32767，使计数频率为1Hz）
        RTC_WaitForLastTask();        // 等待最后一次写操作完成

        MyRTC_SetTime();  // 设置初始时间，将全局数组的值写入RTC

        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);  // 写入首次运行标志
    }
    else  // 非首次运行
    {
        RTC_WaitForSynchro();     // 等待RTC寄存器同步
        RTC_WaitForLastTask();    // 等待最后一次写操作完成
    }
}

// ==================== RTC设置时间函数 ====================

/**
 * @brief 设置RTC时间
 * @note 将全局数组MyRTC_Time的值刷新到RTC硬件寄存器
 * @details 将年、月、日、时、分、秒转换为秒数时间戳，
 *          然后写入RTC的CNT寄存器
 * @note 使用UTC+8（东八区）时区修正
 */
void MyRTC_SetTime(void)
{
    time_t time_cnt;      // 秒数时间戳变量
    struct tm time_date;  // 日历时间结构体

    // 将全局数组的时间赋值给日历时间结构体
    time_date.tm_year = MyRTC_Time[0] - 1900;  // 年份（tm_year是从1900开始的年数）
    time_date.tm_mon = MyRTC_Time[1] - 1;      // 月份（tm_month范围0-11）
    time_date.tm_mday = MyRTC_Time[2];          // 日期
    time_date.tm_hour = MyRTC_Time[3];          // 小时
    time_date.tm_min = MyRTC_Time[4];           // 分钟
    time_date.tm_sec = MyRTC_Time[5];           // 秒

    // 使用mktime函数将日历时间结构体转换为秒数时间戳
    // - 8 * 60 * 60 进行东八区时区减法修正（RTC使用UTC时间）
    time_cnt = mktime(&time_date) - 8 * 60 * 60;

    // 将秒数写入RTC的CNT寄存器
    RTC_SetCounter(time_cnt);
    RTC_WaitForLastTask();  // 等待最后一次写操作完成
}

// ==================== RTC读取时间函数 ====================

/**
 * @brief 读取RTC时间
 * @note 将RTC硬件寄存器的时间值刷新到全局数组MyRTC_Time
 * @details 从RTC的CNT寄存器读取秒数时间戳，
 *          转换为日历时间格式，再分解为数组中的年、月、日、时、分、秒
 * @note 使用UTC+8（东八区）时区修正
 */
void MyRTC_ReadTime(void)
{
    time_t time_cnt;      // 秒数时间戳变量
    struct tm time_date;  // 日历时间结构体

    // 读取RTC的CNT寄存器获取当前的秒数时间戳
    // + 8 * 60 * 60 进行东八区时区加法修正
    time_cnt = RTC_GetCounter() + 8 * 60 * 60;

    // 使用localtime函数将秒数时间戳转换为日历时间格式
    time_date = *localtime(&time_cnt);

    // 将日历时间结构体赋值给全局时间数组
    MyRTC_Time[0] = time_date.tm_year + 1900;  // 年份
    MyRTC_Time[1] = time_date.tm_mon + 1;      // 月份（1-12）
    MyRTC_Time[2] = time_date.tm_mday;         // 日期
    MyRTC_Time[3] = time_date.tm_hour;         // 小时
    MyRTC_Time[4] = time_date.tm_min;          // 分钟
    MyRTC_Time[5] = time_date.tm_sec;          // 秒
}

// ==================== 时间字符串解析函数 ====================

/**
 * @brief 解析时间字符串并设置RTC
 * @param buf 时间字符串，格式为"TIME:YYYY-MM-DD HH:MM:SS"
 * @return 0-成功, -1-失败
 * @note 字符串可以包含前导或尾随的空白字符、冒号等
 *       例如：
 *       - "TIME:2025-08-22 16:40:20"
 *       - ":TIME:2025-08-22 16:40:20\r\n"
 *       - " TIME:2025-08-22 16:40:20 "
 *       都可以正确解析
 * @details 解析步骤：
 *          1. 检查输入参数有效性
 *          2. 复制字符串到临时缓冲区
 *          3. 去除尾部的空白和CR/LF字符
 *          4. 检查是否有"TIME:"前缀
 *          5. 解析年、月、日、时、分、秒
 *          6. 校验时间范围
 *          7. 关中断后写入全局数组并调用MyRTC_SetTime
 *          8. 开中断
 */
int parse_time_and_set_rtc(const char *buf)
{
    if (buf == NULL) return -1;  // 参数无效

    // 1. 复制一份临时字符串以修改，保证安全
    char tmp[64];
    strncpy(tmp, buf, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0';

    // 2. 去掉右边的空白 + CR/LF
    char *p = tmp + strlen(tmp) - 1;
    while (p >= tmp && (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t')) {
        *p = '\0';
        p--;
    }

    // 3. 解析前缀 "TIME:"
    const char *prefix = "TIME:";
    if (strncmp(tmp, prefix, strlen(prefix)) != 0) {
        return -1;  // 前缀不匹配
    }

    // 获取时间字符串部分
    const char *time_str = tmp + strlen(prefix);

    // 4. 时间串可能多一个冒号或空格，去掉前面的 ":" 与空格
    while (*time_str == ':' || *time_str == ' ' || *time_str == '\t') time_str++;

    // 5. 解析年、月、日、时、分、秒
    int yyyy, MM, dd, hh, mm, ss;
    int fields = sscanf(time_str, "%d-%d-%d %d:%d:%d", &yyyy, &MM, &dd, &hh, &mm, &ss);
    if (fields != 6) {
        return -1;  // 解析失败
    }

    // 6. 简单范围校验
    if (yyyy < 1970 || MM < 1 || MM > 12 || dd < 1 || dd > 31 ||
        hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 60) {
        return -1;  // 时间范围无效
    }

    // 7. 先写入全局 MyRTC_Time 数组
    //    由于 MyRTC_Time 可能在 ISR 中被读取，必须关中断防止写到一半时发生中断
    __disable_irq();

    // 参考 MyRTC_SetTime 实现：time_date.tm_year = MyRTC_Time[0] - 1900;
    // 因此 MyRTC_Time[0] 应该是实际年份（如 2025）
    MyRTC_Time[0] = (uint16_t)yyyy;  // 年份
    MyRTC_Time[1] = (uint8_t)MM;     // 月份 1..12
    MyRTC_Time[2] = (uint8_t)dd;     // 日期
    MyRTC_Time[3] = (uint8_t)hh;     // 小时
    MyRTC_Time[4] = (uint8_t)mm;     // 分钟
    MyRTC_Time[5] = (uint8_t)ss;     // 秒

    // 8. 然后将数组的时间写入 RTC
    MyRTC_SetTime();

    __enable_irq();  // 开中断

    return 0;  // 成功
}
