#ifndef DETECTION_TASK_H
#define DETECTION_TASK_H

/**
 ******************************************************************************
 * @file    detection_task.h
 * @brief   环境检测任务头文件
 * @details 本文件提供了环境检测任务的入口函数
 *          负责采集温度、湿度、光照等环境数据，并通过MQTT上报
 ******************************************************************************
 */

// ==================== 函数声明 ====================

/**
 * @brief 环境检测任务入口函数
 * @param param 任务参数（未使用）
 * @details 任务功能：
 *          1. 定期读取光照传感器数据（光敏电阻ADC值）
 *          2. 定期读取DHT11温湿度传感器数据
 *          3. 在在线模式下，每10秒通过MQTT上报一次设备状态
 * @note 上报内容包括：
 *       - 温度（tmp）
 *       - 湿度（hum）
 *       - 光照强度（luminosity）
 *       - 风扇状态（fan_state）
 *       - 窗帘状态（curtain_state）
 *       - LED状态（LED_state）
 *       - 温度阈值（yu_tmp）
 *       - 湿度阈值（yu_hum）
 *       - 光照阈值（yu_light）
 *       - 自动模式状态（auto_mode）
 */
void Detection_Task(void *param);

#endif

