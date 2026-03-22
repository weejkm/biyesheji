#include "stm32f10x.h"                  // Device header
#include <time.h>
#include "MyRTC.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint16_t MyRTC_Time[] = {2025, 8, 22, 16, 40, 20};	//定义全局的时间数组，数组内容分别为年、月、日、时、分、秒

void MyRTC_SetTime(void);				//函数声明

/**
  * 函    数：RTC初始化
  * 参    数：无
  * 返 回 值：无
  */
void MyRTC_Init(void)
{
	/*开启时钟*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);		//开启PWR的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);		//开启BKP的时钟
	
	/*备份寄存器访问使能*/
	PWR_BackupAccessCmd(ENABLE);							//使用PWR开启对备份寄存器的访问
	
	if (BKP_ReadBackupRegister(BKP_DR1) != 0xA5A5)			//通过写入备份寄存器的标志位，判断RTC是否是第一次配置
															//if成立则执行第一次的RTC配置
	{
		RCC_LSEConfig(RCC_LSE_ON);							//开启LSE时钟
		while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != SET);	//等待LSE准备就绪
		
		RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);				//选择RTCCLK来源为LSE
		RCC_RTCCLKCmd(ENABLE);								//RTCCLK使能
		
		RTC_WaitForSynchro();								//等待同步
		RTC_WaitForLastTask();								//等待上一次操作完成
		
		RTC_SetPrescaler(32768 - 1);						//设置RTC预分频器，预分频后的计数频率为1Hz
		RTC_WaitForLastTask();								//等待上一次操作完成
		
		MyRTC_SetTime();									//设置时间，调用此函数，全局数组里时间值刷新到RTC硬件电路
		
		BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);			//在备份寄存器写入自己规定的标志位，用于判断RTC是不是第一次执行配置
	}
	else													//RTC不是第一次配置
	{
		RTC_WaitForSynchro();								//等待同步
		RTC_WaitForLastTask();								//等待上一次操作完成
	}
}

/**
  * 函    数：RTC设置时间
  * 参    数：无
  * 返 回 值：无
  * 说    明：调用此函数后，全局数组里时间值将刷新到RTC硬件电路
  */
void MyRTC_SetTime(void)
{
	time_t time_cnt;		//定义秒计数器数据类型
	struct tm time_date;	//定义日期时间数据类型
	
	time_date.tm_year = MyRTC_Time[0] - 1900;		//将数组的时间赋值给日期时间结构体
	time_date.tm_mon = MyRTC_Time[1] - 1;
	time_date.tm_mday = MyRTC_Time[2];
	time_date.tm_hour = MyRTC_Time[3];
	time_date.tm_min = MyRTC_Time[4];
	time_date.tm_sec = MyRTC_Time[5];
	
	time_cnt = mktime(&time_date) - 8 * 60 * 60;	//调用mktime函数，将日期时间转换为秒计数器格式
													//- 8 * 60 * 60为东八区的时区调整
	
	RTC_SetCounter(time_cnt);						//将秒计数器写入到RTC的CNT中
	RTC_WaitForLastTask();							//等待上一次操作完成
}

/**
  * 函    数：RTC读取时间
  * 参    数：无
  * 返 回 值：无
  * 说    明：调用此函数后，RTC硬件电路里时间值将刷新到全局数组
  */
void MyRTC_ReadTime(void)
{
	time_t time_cnt;		//定义秒计数器数据类型
	struct tm time_date;	//定义日期时间数据类型
	
	time_cnt = RTC_GetCounter() + 8 * 60 * 60;		//读取RTC的CNT，获取当前的秒计数器
													//+ 8 * 60 * 60为东八区的时区调整
	
	time_date = *localtime(&time_cnt);				//使用localtime函数，将秒计数器转换为日期时间格式
	
	MyRTC_Time[0] = time_date.tm_year + 1900;		//将日期时间结构体赋值给数组的时间
	MyRTC_Time[1] = time_date.tm_mon + 1;
	MyRTC_Time[2] = time_date.tm_mday;
	MyRTC_Time[3] = time_date.tm_hour;
	MyRTC_Time[4] = time_date.tm_min;
	MyRTC_Time[5] = time_date.tm_sec;
}

/*
 * parse_time_and_set_rtc
 *  解析 "TIME:YYYY-MM-DD HH:MM:SS"（允许末尾有 \r\n）
 *  成功返回 0，失败返回 -1
 */
int parse_time_and_set_rtc(const char *buf)
{
    if (buf == NULL) return -1;

    /* 复制一份本地字符串做修改（安全） */
    char tmp[64];
    strncpy(tmp, buf, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0';

    /* 去掉前后空白和 CR/LF */
    // trim right CR/LF
    char *p = tmp + strlen(tmp) - 1;
    while (p >= tmp && (*p == '\r' || *p == '\n' || *p == ' ' || *p == '\t')) {
        *p = '\0';
        p--;
    }

    /* 检查前缀 */
    const char *prefix = "TIME:";
    if (strncmp(tmp, prefix, strlen(prefix)) != 0) {
        return -1;
    }

    const char *time_str = tmp + strlen(prefix);

    /* 有时可能多一个冒号（比如日志中出现 ":TIME:..."），容错去掉前导 ':' 或空格 */
    while (*time_str == ':' || *time_str == ' ' || *time_str == '\t') time_str++;

    int yyyy, MM, dd, hh, mm, ss;
    int fields = sscanf(time_str, "%d-%d-%d %d:%d:%d", &yyyy, &MM, &dd, &hh, &mm, &ss);
    if (fields != 6) {
        return -1;
    }

    /* 简单范围校验（可根据需要更严格） */
    if (yyyy < 1970 || MM < 1 || MM > 12 || dd < 1 || dd > 31 ||
        hh < 0 || hh > 23 || mm < 0 || mm > 59 || ss < 0 || ss > 60) {
        return -1;
    }

    /* 将结果写入全局 MyRTC_Time 并调用设置函数
       如果 MyRTC_Time 被 ISR 或其它任务访问，最好在写和调用时短暂屏蔽中断 */
    __disable_irq();

    /* 根据你 MyRTC_SetTime 实现，time_date.tm_year = MyRTC_Time[0] - 1900;
       所以 MyRTC_Time[0] 应保存完整公历年（例如 2025） */
    MyRTC_Time[0] = (uint16_t)yyyy;   // year
    MyRTC_Time[1] = (uint8_t)MM;      // month 1..12
    MyRTC_Time[2] = (uint8_t)dd;      // day
    MyRTC_Time[3] = (uint8_t)hh;      // hour
    MyRTC_Time[4] = (uint8_t)mm;      // minute
    MyRTC_Time[5] = (uint8_t)ss;      // second

    /* 调用你的函数将时间写入 RTC */
    MyRTC_SetTime();

    __enable_irq();

    return 0;
}
