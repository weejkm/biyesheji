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

/* ====================== MQTT/WiFi (ThingsCloud) ====================== */
#ifndef WIFI_SSID
#define WIFI_SSID      "bin"
#endif

/* 统一名称：我后面代码用 WIFI_PWD，所以这里把 WIFI_PWD 定义出来 */
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

/* 统一名称：我后面代码用 MQTT_USER / MQTT_PASS */
#ifndef MQTT_USER
#define MQTT_USER      "5o6nhxbmqk80h82d"
#endif

#ifndef MQTT_PASS
#define MQTT_PASS      "sIxS9ob7Qm"
#endif

/* ThingsCloud 主题 */
#ifndef MQTT_TOPIC_PUB
#define MQTT_TOPIC_PUB        "attributes"          /* 设备上报属性 */
#endif

#ifndef MQTT_TOPIC_SUB_PUSH
#define MQTT_TOPIC_SUB_PUSH   "attributes/push"     /* 云端下发属性 */
#endif

#ifndef MQTT_TOPIC_SUB_RESP
#define MQTT_TOPIC_SUB_RESP   "attributes/response" /* 上报应答 */
#endif

/* link_id / scheme */
#ifndef MQTT_LINK_ID
#define MQTT_LINK_ID   ((uint8_t)0)
#endif

#ifndef MQTT_SCHEME
#define MQTT_SCHEME    ((uint8_t)1)  /* 以你 ESP8266.h 的 scheme 定义为准 */
#endif

/* 超时/重试 */
#ifndef WIFI_TIMEOUT_MS
#define WIFI_TIMEOUT_MS   15000u
#endif
#ifndef WIFI_RETRY
#define WIFI_RETRY        2u
#endif

#ifndef MQTT_TIMEOUT_MS
#define MQTT_TIMEOUT_MS   8000u
#endif
#ifndef MQTT_RETRY
#define MQTT_RETRY        2u
#endif
#ifndef MQTT_RECONNECT
#define MQTT_RECONNECT    1u
#endif

#ifndef PUB_TIMEOUT_MS
#define PUB_TIMEOUT_MS    5000u
#endif


/* ========== 小工具：弱 JSON 提取（与你 main.c 的逻辑一致） ========== */
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

/* ========== ESP8266 订阅消息回调：ESP8266.c 会在收到 +MQTTSUBRECV 后触发 ========== */
static void on_mqtt_subrecv(uint8_t link_id, const char *topic, const char *payload)
{
    (void)link_id;
    if (!topic || !payload) return;

    ESP8266_HandleCommand(topic, payload);
}

/* ========== 连接 + 订阅 ========== */
static uint8_t mqtt_full_connect(void)
{
    wifi_t wifi;
    mqtt_usercfg_t cfg;

    /* 1) WiFi */
    wifi.ssid = WIFI_SSID;
    wifi.password = WIFI_PWD;

    if (!ESP8266_WiFi_Connect(&wifi, WIFI_TIMEOUT_MS, WIFI_RETRY)) {
        return 0;
    }

    /* 等待状态变 OK（URC: WIFI GOT IP） */
    for (int i = 0; i < 300; i++) {
        ESP8266_Process();
        if (ESP8266_IsWiFiOK()) break;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!ESP8266_IsWiFiOK()) return 0;

    /* 2) MQTT 用户配置 */
    cfg.link_id   = MQTT_LINK_ID;
    cfg.scheme    = MQTT_SCHEME;
    cfg.client_id = MQTT_CLIENT_ID;
    cfg.username  = MQTT_USER;
    cfg.password  = MQTT_PASS;

    if (!ESP8266_MQTT_UserCfg(&cfg, MQTT_TIMEOUT_MS)) {
        return 0;
    }

    /* 3) MQTT 连接 */
    if (!ESP8266_MQTT_Connect(MQTT_LINK_ID, MQTT_HOST, (uint16_t)MQTT_PORT,
                             MQTT_RECONNECT, MQTT_TIMEOUT_MS, MQTT_RETRY)) {
        return 0;
    }

    /* 等待 MQTT OK（URC: +MQTTCONNECTED:） */
    for (int i = 0; i < 300; i++) {
        ESP8266_Process();
        if (ESP8266_IsMQTTOK()) break;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!ESP8266_IsMQTTOK()) return 0;

    /* 4) 订阅（注意：你的 Subscribe 没有 callback 参数） */
	ESP8266_MQTT_Subscribe(MQTT_LINK_ID, MQTT_TOPIC_SUB_PUSH, 0, MQTT_TIMEOUT_MS);
	ESP8266_MQTT_Subscribe(MQTT_LINK_ID, MQTT_TOPIC_SUB_RESP, 0, MQTT_TIMEOUT_MS);

    /* 5) 请求云端时间一次 */
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

/* ========== 云端下发处理：与之前逻辑一致，但调用你工程真实驱动 ========== */
void ESP8266_HandleCommand(const char *topic, const char *json)
{
    if (!topic || !json) return;

    if (strstr(topic, "attributes/push")) {

        float f = 0.0f;
        uint64_t ts = 0;
        uint8_t changed_threshold = 0;

        /* 阈值 */
        if (json_get_float(json, "\"yu_tmp\"", &f))   { set_temp_threshold  = (uint8_t)f; changed_threshold = 1; }
        if (json_get_float(json, "\"yu_hum\"", &f))   { set_hum_threshold   = (uint8_t)f; changed_threshold = 1; }
        if (json_get_float(json, "\"yu_light\"", &f)) { set_light_threshold = (uint8_t)f; changed_threshold = 1; }

        /* 风扇 */
        if (json_get_float(json, "\"fan_state\"", &f)) {
            dev_fan_state = (uint8_t)f;
            Fan_set(dev_fan_state);
        }

        /* 窗帘 */
        if (json_get_float(json, "\"curtain_state\"", &f)) {
            dev_curtain_state = (uint8_t)f;
            if (dev_curtain_state) {
                Servo_SetON;    /* 你的 Servo.h 是宏语句，不能写成 {Servo_SetON;} */
            } else {
                Servo_SetOFF;
            }
        }

        /* 灯亮度 */
        if (json_get_float(json, "\"LED_state\"", &f)) {
            dev_led_level = (uint8_t)f;
            if (dev_led_level > 100) dev_led_level = 100;
            LED_Set_light(dev_led_level);
        }

        /* 校时：ts(ms) */
        if (!g_time_synced && json_get_u64(json, "\"ts\"", &ts)) {
            RTC_SetCounter((uint32_t)(ts / 1000ULL));
            g_time_synced = 1;
        }

        /* 阈值变化回报 */
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

        /* 云端下发 => 切手动 */
        g_auto_mode = 0;
    }
}

/* ========== MQTT 任务：轮询 ESP8266_Process + 发布队列 ========== */
void EspMqtt_Task(void *arg)
{
    (void)arg;

    /* 注册订阅回调（靠 ESP8266_Process 解析 URC 后触发） */
    ESP8266_SetMqttMsgCallback(on_mqtt_subrecv);

    /* 先连一次 */
    (void)mqtt_full_connect();

    while (1) {

        /* 消费 URC（包括 WIFI/MQTT 状态变化、订阅消息） */
        ESP8266_Process();

        /* 掉线重连（你 ESP8266.c 里 WIFI DISCONNECT 会把 mqtt_ok 清 0） */
        if (!ESP8266_IsWiFiOK() || !ESP8266_IsMQTTOK()) {
            (void)mqtt_full_connect();
        }

        /* 发布队列 */
        if (g_mqtt_pub_q) {
            mqtt_pub_msg_t m;
            if (xQueueReceive(g_mqtt_pub_q, &m, pdMS_TO_TICKS(100)) == pdPASS) {
                if (ESP8266_IsMQTTOK()) {
                    (void)ESP8266_MQTT_Publish(MQTT_LINK_ID,
                                              m.topic,
                                              m.payload,
                                              m.qos,
                                              m.retain,
                                              PUB_TIMEOUT_MS);
                } else {
                    /* MQTT 不OK则塞回队头，稍后重试 */
                    xQueueSendToFront(g_mqtt_pub_q, &m, 0);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
