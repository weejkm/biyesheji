#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

#include "ESP8266.h"
#include "MyRTC.h"
#include "Fan.h"
#include "LED.h"
#include "Servo.h"

#include "app_state.h"
#include "mqtt_task.h"

/* ====================== MQTT / WiFi 配置（ThingsCloud） ====================== */
#ifndef WIFI_SSID
#define WIFI_SSID      "bin"
#endif

#ifndef WIFI_PWD
#define WIFI_PWD       "88888888"
#endif

#ifndef MQTT_HOST
#define MQTT_HOST      "sh-5-mqtt.iot-api.com"
#endif

#ifndef MQTT_PORT
#define MQTT_PORT      1883
#endif

#ifndef MQTT_CLIENT_ID
#define MQTT_CLIENT_ID "ESP01"
#endif

#ifndef MQTT_USER
#define MQTT_USER      "5o6nhxbmqk80h82d"
#endif

#ifndef MQTT_PASS
#define MQTT_PASS      "sIxS9ob7Qm"
#endif

/* ThingsCloud 主题 */
#ifndef MQTT_TOPIC_PUB
#define MQTT_TOPIC_PUB        "attributes"          /* 设备属性上报主题 */
#endif

#ifndef MQTT_TOPIC_SUB_PUSH
#define MQTT_TOPIC_SUB_PUSH   "attributes/push"     /* 云端属性下发主题 */
#endif

#ifndef MQTT_TOPIC_SUB_RESP
#define MQTT_TOPIC_SUB_RESP   "attributes/response" /* 云端属性响应主题 */
#endif

/* MQTT 连接参数 */
#ifndef MQTT_LINK_ID
#define MQTT_LINK_ID   ((uint8_t)0)
#endif

#ifndef MQTT_SCHEME
#define MQTT_SCHEME    ((uint8_t)1)  /* 以 ESP8266.h 中定义的 scheme 为准 */
#endif

/* WiFi 连接超时与重试参数 */
#ifndef WIFI_TIMEOUT_MS
#define WIFI_TIMEOUT_MS   15000u
#endif
#ifndef WIFI_RETRY
#define WIFI_RETRY        2u
#endif

/* MQTT 连接超时与重试参数 */
#ifndef MQTT_TIMEOUT_MS
#define MQTT_TIMEOUT_MS   8000u
#endif
#ifndef MQTT_RETRY
#define MQTT_RETRY        2u
#endif
#ifndef MQTT_RECONNECT
#define MQTT_RECONNECT    1u
#endif

/* MQTT 发布超时时间 */
#ifndef PUB_TIMEOUT_MS
#define PUB_TIMEOUT_MS    5000u
#endif

/* WiFi / MQTT 掉线判定的防抖计数 */
#ifndef MQTT_LOST_DEBOUNCE_COUNT
#define MQTT_LOST_DEBOUNCE_COUNT  20u
#endif
#ifndef WIFI_LOST_DEBOUNCE_COUNT
#define WIFI_LOST_DEBOUNCE_COUNT  20u
#endif

/* MQTT 重连退避参数 */
#ifndef MQTT_RECONNECT_BACKOFF_MIN_MS
#define MQTT_RECONNECT_BACKOFF_MIN_MS  3000u
#endif
#ifndef MQTT_RECONNECT_BACKOFF_MAX_MS
#define MQTT_RECONNECT_BACKOFF_MAX_MS  15000u
#endif

/* 连续失败达到阈值后，重新初始化 ESP8266 */
#ifndef MQTT_REINIT_FAIL_THRESHOLD
#define MQTT_REINIT_FAIL_THRESHOLD  3u
#endif

/* 离线状态切换控制参数 */
#ifndef MQTT_OFFLINE_FAIL_THRESHOLD
#define MQTT_OFFLINE_FAIL_THRESHOLD  4u
#endif
#ifndef MQTT_OFFLINE_SWITCH_TIMEOUT_MS
#define MQTT_OFFLINE_SWITCH_TIMEOUT_MS  30000u
#endif
#ifndef MQTT_OFFLINE_FORCE_TIMEOUT_MS
#define MQTT_OFFLINE_FORCE_TIMEOUT_MS  45000u
#endif


/* =========================================================================
 * 简易 JSON 解析函数
 * 说明：
 * 这里没有使用完整 JSON 库，而是通过字符串查找 + sscanf 的方式提取键值。
 * 适合结构简单、字段固定的场景。
 * ========================================================================= */

/**
 * @brief 从 JSON 字符串中提取 float 类型数值
 * @param json  输入的 JSON 字符串
 * @param key   要查找的键名，例如 "\"yu_tmp\""
 * @param out   输出结果指针
 * @return 1 表示提取成功，0 表示失败
 */
static uint8_t json_get_float(const char *json, const char *key, float *out)
{
    const char *p = strstr(json, key);
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p) return 0;
    p++;

    while (*p == ' ' || *p == '\t') p++;

    return (sscanf(p, "%f", out) == 1);
}

/**
 * @brief 从 JSON 字符串中提取 uint64_t 类型数值
 * @param json  输入的 JSON 字符串
 * @param key   要查找的键名，例如 "\"ts\""
 * @param out   输出结果指针
 * @return 1 表示提取成功，0 表示失败
 */
static uint8_t json_get_u64(const char *json, const char *key, uint64_t *out)
{
    const char *p = strstr(json, key);
    if (!p) return 0;

    p = strchr(p, ':');
    if (!p) return 0;
    p++;

    while (*p == ' ' || *p == '\t') p++;

    unsigned long long v = 0;
    if (sscanf(p, "%llu", &v) == 1) {
        *out = (uint64_t)v;
        return 1;
    }
    return 0;
}

/* =========================================================================
 * MQTT 订阅消息回调
 * 说明：
 * 当 ESP8266 模块收到 +MQTTSUBRECV 事件后，会调用这里注册的回调函数。
 * 在该回调里统一转给 ESP8266_HandleCommand() 做业务处理。
 * ========================================================================= */
static void on_mqtt_subrecv(uint8_t link_id, const char *topic, const char *payload)
{
    (void)link_id;
    if (!topic || !payload) return;

    ESP8266_HandleCommand(topic, payload);
}

/* =========================================================================
 * 仅连接 WiFi
 * 说明：
 * 1. 如果已经连上 WiFi，直接返回成功。
 * 2. 否则调用 ESP8266_WiFi_Connect() 建立连接。
 * 3. 随后轮询 ESP8266_Process()，等待模块状态真正变为已连接。
 * ========================================================================= */
static uint8_t wifi_connect_only(void)
{
    wifi_t wifi;

    if (ESP8266_IsWiFiOK()) return 1;

    wifi.ssid = WIFI_SSID;
    wifi.password = WIFI_PWD;

    if (!ESP8266_WiFi_Connect(&wifi, WIFI_TIMEOUT_MS, WIFI_RETRY)) {
        return 0;
    }

    for (int i = 0; i < 300; i++) {
        ESP8266_Process();
        if (ESP8266_IsWiFiOK()) return 1;
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return 0;
}

/* =========================================================================
 * 仅连接 MQTT
 * 说明：
 * 1. 前提必须是 WiFi 已连接。
 * 2. 如果 MQTT 已连接，直接返回成功。
 * 3. 先清理旧连接，再配置 MQTT 用户参数。
 * 4. 建立 MQTT 连接，并订阅 push / response 两个主题。
 * 5. 连接成功后，向发布队列中投递一个 {"get_time":1}，
 *    用于向云端请求当前时间戳，便于 RTC 校时。
 * ========================================================================= */
static uint8_t mqtt_connect_only(void)
{
    mqtt_usercfg_t cfg;

    if (!ESP8266_IsWiFiOK()) return 0;
    if (ESP8266_IsMQTTOK()) return 1;

    cfg.link_id   = MQTT_LINK_ID;
    cfg.scheme    = MQTT_SCHEME;
    cfg.client_id = MQTT_CLIENT_ID;
    cfg.username  = MQTT_USER;
    cfg.password  = MQTT_PASS;

    /* 先清理旧的 MQTT 连接状态，避免重复连接异常 */
    (void)ESP8266_MQTT_Clean(MQTT_LINK_ID, 1000);
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP8266_Process();

    /* 配置 MQTT 用户信息 */
    if (!ESP8266_MQTT_UserCfg(&cfg, MQTT_TIMEOUT_MS)) {
        return 0;
    }

    /* 发起 MQTT 连接 */
    if (!ESP8266_MQTT_Connect(MQTT_LINK_ID, MQTT_HOST, (uint16_t)MQTT_PORT,
                             MQTT_RECONNECT, MQTT_TIMEOUT_MS, MQTT_RETRY)) {
        return 0;
    }

    /* 等待 MQTT 状态稳定 */
    for (int i = 0; i < 300; i++) {
        ESP8266_Process();
        if (ESP8266_IsMQTTOK()) break;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!ESP8266_IsMQTTOK()) return 0;

    /* 订阅云端下发主题 */
    if (!ESP8266_MQTT_Subscribe(MQTT_LINK_ID, MQTT_TOPIC_SUB_PUSH, 0, MQTT_TIMEOUT_MS)) return 0;
    if (!ESP8266_MQTT_Subscribe(MQTT_LINK_ID, MQTT_TOPIC_SUB_RESP, 0, MQTT_TIMEOUT_MS)) return 0;

    /* 连接成功后请求云端下发时间戳，用于 RTC 校时 */
    if (g_mqtt_pub_q) {
        mqtt_pub_msg_t m;
        memset(&m, 0, sizeof(m));
        strcpy(m.topic, MQTT_TOPIC_PUB);
        strcpy(m.payload, "{\"get_time\":1}");
        m.qos = 0;
        m.retain = 0;
        xQueueSend(g_mqtt_pub_q, &m, 0);
    }

    return 1;
}

/* =========================================================================
 * WiFi + MQTT 完整连接流程
 * ========================================================================= */
static uint8_t mqtt_full_connect(void)
{
    if (!wifi_connect_only()) return 0;
    if (!mqtt_connect_only()) return 0;
    return 1;
}

/* =========================================================================
 * 处理云端下发的控制命令
 * 说明：
 * 仅处理 "attributes/push" 主题消息。
 *
 * 支持的字段：
 * - yu_tmp       : 温度阈值
 * - yu_hum       : 湿度阈值
 * - yu_light     : 光照阈值
 * - fan_state    : 风扇开关状态
 * - curtain_state: 窗帘开关状态
 * - LED_state    : LED 亮度（0~100）
 * - ts           : 云端下发时间戳（毫秒），用于 RTC 校时
 *
 * 处理逻辑：
 * 1. 更新阈值类参数；
 * 2. 执行设备控制；
 * 3. 如果首次收到时间戳且本地尚未校时，则写入 RTC；
 * 4. 若阈值发生变化，则主动回传新的阈值给云端；
 * 5. 收到云端控制后，将系统切换为手动模式（g_auto_mode = 0）。
 * ========================================================================= */
void ESP8266_HandleCommand(const char *topic, const char *json)
{
    if (!topic || !json) return;

    if (strstr(topic, "attributes/push")) {

        float f = 0.0f;
        uint64_t ts = 0;
        uint8_t changed_threshold = 0;

        /* 处理阈值类参数 */
        if (json_get_float(json, "\"yu_tmp\"", &f))   { set_temp_threshold  = (uint8_t)f; changed_threshold = 1; }
        if (json_get_float(json, "\"yu_hum\"", &f))   { set_hum_threshold   = (uint8_t)f; changed_threshold = 1; }
        if (json_get_float(json, "\"yu_light\"", &f)) { set_light_threshold = (uint8_t)f; changed_threshold = 1; }

        /* 处理风扇状态 */
        if (json_get_float(json, "\"fan_state\"", &f)) {
            dev_fan_state = (uint8_t)f;
            Fan_set(dev_fan_state);
        }

        /* 处理窗帘状态 */
        if (json_get_float(json, "\"curtain_state\"", &f)) {
            dev_curtain_state = (uint8_t)f;
            if (dev_curtain_state) {
                Servo_SetON;    /* 如果 Servo.h 中定义为宏，则保持这种写法 */
            } else {
                Servo_SetOFF;
            }
        }

        /* 处理 LED 亮度 */
        if (json_get_float(json, "\"LED_state\"", &f)) {
            dev_led_level = (uint8_t)f;
            if (dev_led_level > 100) dev_led_level = 100;
            LED_Set_light(dev_led_level);
        }

        /* 处理时间戳：仅在未校时的情况下使用一次 */
        if (!g_time_synced && json_get_u64(json, "\"ts\"", &ts)) {
            RTC_SetCounter((uint32_t)(ts / 1000ULL)); /* 云端是毫秒，RTC 使用秒 */
            g_time_synced = 1;
        }

        /* 若阈值变化，则将最新阈值主动回传云端 */
        if (changed_threshold && g_mqtt_pub_q) {
            mqtt_pub_msg_t m;
            memset(&m, 0, sizeof(m));
            strcpy(m.topic, "attributes");
            snprintf(m.payload, sizeof(m.payload),
                     "{\"yu_tmp\":%u,\"yu_hum\":%u,\"yu_light\":%u}",
                     set_temp_threshold, set_hum_threshold, set_light_threshold);
            m.qos = 0;
            m.retain = 0;
            xQueueSend(g_mqtt_pub_q, &m, 0);
        }

        /* 收到云端控制后，切换为手动模式 */
        g_auto_mode = 0;
    }
}

/* =========================================================================
 * MQTT 任务
 *
 * 功能说明：
 * 1. 周期性调用 ESP8266_Process() 驱动串口协议状态机；
 * 2. 监测 WiFi / MQTT 在线状态；
 * 3. 出现掉线时，执行带退避机制的自动重连；
 * 4. 当连续失败过多或离线过久时，自动切换到离线模式；
 * 5. 在线模式下，从消息队列读取待发布消息并发送到 MQTT。
 *
 * 设计特点：
 * - 使用防抖计数，避免短暂波动引起误判掉线；
 * - 使用退避重连，降低频繁重连对模块和网络的冲击；
 * - 连续失败达到阈值后，重新初始化 ESP8266；
 * - 若长时间无法恢复在线，则自动退出在线模式。
 * ========================================================================= */
void EspMqtt_Task(void *arg)
{
    uint8_t wifi_lost_cnt = 0;          /* WiFi 掉线防抖计数 */
    uint8_t mqtt_lost_cnt = 0;          /* MQTT 掉线防抖计数 */
    uint8_t reconnect_fail_cnt = 0;     /* 连续重连失败计数（用于触发模块重初始化） */
    uint8_t offline_total_fail_cnt = 0; /* 离线期间累计失败次数 */
    uint8_t offline_disconnect_done = 0;/* 离线模式下是否已执行过断开动作 */
    uint8_t last_online_mode;           /* 记录上一次在线模式状态 */
    TickType_t next_reconnect_tick = 0; /* 下一次允许执行重连的时刻 */
    TickType_t reconnect_start_tick = 0;/* 本轮掉线恢复流程的开始时刻 */
    uint32_t reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MIN_MS; /* 当前退避时长 */

    (void)arg;

    /* 注册 MQTT 订阅消息回调 */
    ESP8266_SetMqttMsgCallback(on_mqtt_subrecv);
    last_online_mode = g_online_mode;

    /* 如果系统初始处于在线模式，启动时先尝试连接 */
    if (g_online_mode) {
        if (mqtt_full_connect()) {
            next_reconnect_tick = xTaskGetTickCount();
        } else {
            reconnect_start_tick = xTaskGetTickCount();
            next_reconnect_tick = reconnect_start_tick + pdMS_TO_TICKS(reconnect_backoff_ms);
        }
    }

    while (1) {
        /* 驱动 ESP8266 状态机 */
        ESP8266_Process();

        /* 如果在线/离线模式发生变化，则清理掉线检测与重连状态 */
        if (g_online_mode != last_online_mode) {
            wifi_lost_cnt = 0;
            mqtt_lost_cnt = 0;
            reconnect_fail_cnt = 0;
            offline_total_fail_cnt = 0;
            reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MIN_MS;
            reconnect_start_tick = 0;
            next_reconnect_tick = xTaskGetTickCount();
            offline_disconnect_done = 0;
            last_online_mode = g_online_mode;
        }

        /* ===================== 离线模式处理 ===================== */
        if (!g_online_mode) {
            /* 刚切到离线模式时，只执行一次断开操作 */
            if (!offline_disconnect_done) {
                (void)ESP8266_MQTT_Clean(MQTT_LINK_ID, 1000);
                vTaskDelay(pdMS_TO_TICKS(100));
                (void)ESP8266_WiFi_Disconnect(1000);
                offline_disconnect_done = 1;
            }

            /* 清空待发送队列，避免离线期间消息堆积 */
            if (g_mqtt_pub_q) {
                mqtt_pub_msg_t drop_msg;
                while (xQueueReceive(g_mqtt_pub_q, &drop_msg, 0) == pdPASS) {
                }
            }

            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        /* 在线模式下，允许重新进行连接流程 */
        offline_disconnect_done = 0;

        /* 如果本轮重连持续时间过长，则强制切换到离线模式 */
        if (reconnect_start_tick != 0) {
            TickType_t offline_elapsed = xTaskGetTickCount() - reconnect_start_tick;
            if (offline_elapsed >= pdMS_TO_TICKS(MQTT_OFFLINE_FORCE_TIMEOUT_MS)) {
                g_online_mode = 0;
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
        }

        /* ===================== 在线状态检测 ===================== */
        if (!ESP8266_IsWiFiOK()) {
            /* WiFi 不在线时，只累计 WiFi 掉线计数 */
            if (wifi_lost_cnt < 255u) wifi_lost_cnt++;
            mqtt_lost_cnt = 0;
        } else {
            wifi_lost_cnt = 0;

            if (!ESP8266_IsMQTTOK()) {
                /* WiFi 正常但 MQTT 不在线时，累计 MQTT 掉线计数 */
                if (mqtt_lost_cnt < 255u) mqtt_lost_cnt++;
            } else {
                /* WiFi 和 MQTT 都正常，清空所有异常恢复状态 */
                mqtt_lost_cnt = 0;
                reconnect_fail_cnt = 0;
                offline_total_fail_cnt = 0;
                reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MIN_MS;
                reconnect_start_tick = 0;
                next_reconnect_tick = xTaskGetTickCount();
            }
        }

        /* ===================== 掉线恢复处理 ===================== */
        if ((wifi_lost_cnt >= WIFI_LOST_DEBOUNCE_COUNT) ||
            (ESP8266_IsWiFiOK() && mqtt_lost_cnt >= MQTT_LOST_DEBOUNCE_COUNT)) {

            TickType_t now = xTaskGetTickCount();

            /* 第一次进入恢复流程时，记录开始时间 */
            if (reconnect_start_tick == 0) {
                reconnect_start_tick = now;
            }

            /* 到达允许重连的时间点时，执行重连 */
            if ((int32_t)(now - next_reconnect_tick) >= 0) {
                uint8_t ok;

                /* WiFi 掉线则需要完整重连；否则只重连 MQTT */
                if (wifi_lost_cnt >= WIFI_LOST_DEBOUNCE_COUNT) {
                    ok = mqtt_full_connect();
                } else {
                    ok = mqtt_connect_only();
                }

                vTaskDelay(pdMS_TO_TICKS(20));

                if (ok) {
                    /* 恢复成功，清空错误状态 */
                    wifi_lost_cnt = 0;
                    mqtt_lost_cnt = 0;
                    reconnect_fail_cnt = 0;
                    offline_total_fail_cnt = 0;
                    reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MIN_MS;
                    reconnect_start_tick = 0;
                    next_reconnect_tick = now;
                } else {
                    TickType_t offline_elapsed;

                    reconnect_fail_cnt++;
                    offline_total_fail_cnt++;

                    /* 重连失败后增加退避时间，直到达到上限 */
                    if (reconnect_backoff_ms < MQTT_RECONNECT_BACKOFF_MAX_MS) {
                        reconnect_backoff_ms += MQTT_RECONNECT_BACKOFF_MIN_MS;
                        if (reconnect_backoff_ms > MQTT_RECONNECT_BACKOFF_MAX_MS) {
                            reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MAX_MS;
                        }
                    }

                    next_reconnect_tick = now + pdMS_TO_TICKS(reconnect_backoff_ms);

                    /* 连续多次失败后，重新初始化 ESP8266 模块 */
                    if (reconnect_fail_cnt >= MQTT_REINIT_FAIL_THRESHOLD) {
                        ESP8266_Init(115200);
                        reconnect_fail_cnt = 0;
                        wifi_lost_cnt = WIFI_LOST_DEBOUNCE_COUNT;
                        mqtt_lost_cnt = 0;
                    }

                    /* 统计离线持续时长 */
                    offline_elapsed = xTaskGetTickCount() - reconnect_start_tick;

                    /*
                     * 切换到离线模式的条件：
                     * 1. 离线时间 >= 30s 且累计失败次数达到阈值；
                     * 2. 离线时间 >= 45s，则无条件强制切换离线。
                     */
                    if (((offline_elapsed >= pdMS_TO_TICKS(MQTT_OFFLINE_SWITCH_TIMEOUT_MS)) &&
                         (offline_total_fail_cnt >= MQTT_OFFLINE_FAIL_THRESHOLD)) ||
                        (offline_elapsed >= pdMS_TO_TICKS(MQTT_OFFLINE_FORCE_TIMEOUT_MS))) {
                        g_online_mode = 0;
                    }
                }
            }
        }

        /* ===================== 发布消息处理 ===================== */
        if (g_mqtt_pub_q) {
            mqtt_pub_msg_t m;
            if (xQueueReceive(g_mqtt_pub_q, &m, pdMS_TO_TICKS(100)) == pdPASS) {
                if (g_online_mode && ESP8266_IsMQTTOK()) {
                    if (!ESP8266_MQTT_Publish(MQTT_LINK_ID,
                                              m.topic,
                                              m.payload,
                                              m.qos,
                                              m.retain,
                                              PUB_TIMEOUT_MS)) {
                        /* 发布失败时，把消息重新塞回队列头部，等待后续重试 */
                        xQueueSendToFront(g_mqtt_pub_q, &m, 0);
                        mqtt_lost_cnt = MQTT_LOST_DEBOUNCE_COUNT;
                    }
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
