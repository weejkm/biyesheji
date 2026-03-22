#ifndef __MQTT_TASK_H
#define __MQTT_TASK_H

/**
 ******************************************************************************
 * @file    mqtt_task.h
 * @brief   MQTT通信任务头文件
 * @details 本文件提供了MQTT任务的入口函数和云平台指令处理函数
 *          负责WiFi连接、MQTT连接、消息收发和指令处理
 ******************************************************************************
 */

#include <stdint.h>
#include "app_state.h"   // mqtt_pub_msg_t与g_mqtt_pub_q定义或引用全局状态

// ==================== 函数声明 ====================

/**
 * @brief MQTT/WiFi任务入口函数
 * @param arg 任务参数（未使用）
 * @details 任务功能：
 *          1. 管理WiFi连接状态
 *          2. 管理MQTT连接状态
 *          3. 实现自动重连机制（指数退避）
 *          4. 处理MQTT消息收发
 *          5. 处理云平台下发的控制指令
 */
void EspMqtt_Task(void *arg);

/**
 * @brief 处理云平台推送的指令
 * @param topic MQTT主题名称
 * @param json JSON格式的指令内容
 * @details 支持的指令：
 *          - 设备控制：风扇开关、窗帘开关、灯光亮度
 *          - 阈值设置：温度阈值、湿度阈值、光照阈值
 *          - 时间同步：通过ts字段同步RTC时间
 */
void ESP8266_HandleCommand(const char *topic, const char *json);

#endif

