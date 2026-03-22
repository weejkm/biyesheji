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

/* ???? WIFI_PWD WIFI_PWD  */
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

/* ???? MQTT_USER / MQTT_PASS */
#ifndef MQTT_USER
#define MQTT_USER      "5o6nhxbmqk80h82d"
#endif

#ifndef MQTT_PASS
#define MQTT_PASS      "sIxS9ob7Qm"
#endif

/* ThingsCloud  */
#ifndef MQTT_TOPIC_PUB
#define MQTT_TOPIC_PUB        "attributes"          /* ıÙ??*/
#endif

#ifndef MQTT_TOPIC_SUB_PUSH
#define MQTT_TOPIC_SUB_PUSH   "attributes/push"     /* ?°§ */
#endif

#ifndef MQTT_TOPIC_SUB_RESP
#define MQTT_TOPIC_SUB_RESP   "attributes/response" /* ?? */
#endif

/* link_id / scheme */
#ifndef MQTT_LINK_ID
#define MQTT_LINK_ID   ((uint8_t)0)
#endif

#ifndef MQTT_SCHEME
#define MQTT_SCHEME    ((uint8_t)1)  /*  ESP8266.h  scheme ?? */
#endif

/* ?/ */
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

#ifndef MQTT_LOST_DEBOUNCE_COUNT
#define MQTT_LOST_DEBOUNCE_COUNT  20u
#endif
#ifndef WIFI_LOST_DEBOUNCE_COUNT
#define WIFI_LOST_DEBOUNCE_COUNT  20u
#endif
#ifndef MQTT_RECONNECT_BACKOFF_MIN_MS
#define MQTT_RECONNECT_BACKOFF_MIN_MS  3000u
#endif
#ifndef MQTT_RECONNECT_BACKOFF_MAX_MS
#define MQTT_RECONNECT_BACKOFF_MAX_MS  15000u
#endif
#ifndef MQTT_REINIT_FAIL_THRESHOLD
#define MQTT_REINIT_FAIL_THRESHOLD  3u
#endif


/* ========== ß≥? JSON ? main.c ??? ========== */
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

/* ========== ESP8266 ??ESP8266.c ? +MQTTSUBRECV ? ========== */
static void on_mqtt_subrecv(uint8_t link_id, const char *topic, const char *payload)
{
    (void)link_id;
    if (!topic || !payload) return;

    ESP8266_HandleCommand(topic, payload);
}

/* ==========  +  ========== */
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

    (void)ESP8266_MQTT_Clean(MQTT_LINK_ID, 1000);
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP8266_Process();

    if (!ESP8266_MQTT_UserCfg(&cfg, MQTT_TIMEOUT_MS)) {
        return 0;
    }

    if (!ESP8266_MQTT_Connect(MQTT_LINK_ID, MQTT_HOST, (uint16_t)MQTT_PORT,
                             MQTT_RECONNECT, MQTT_TIMEOUT_MS, MQTT_RETRY)) {
        return 0;
    }

    for (int i = 0; i < 300; i++) {
        ESP8266_Process();
        if (ESP8266_IsMQTTOK()) break;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (!ESP8266_IsMQTTOK()) return 0;

    if (!ESP8266_MQTT_Subscribe(MQTT_LINK_ID, MQTT_TOPIC_SUB_PUSH, 0, MQTT_TIMEOUT_MS)) return 0;
    if (!ESP8266_MQTT_Subscribe(MQTT_LINK_ID, MQTT_TOPIC_SUB_RESP, 0, MQTT_TIMEOUT_MS)) return 0;

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

static uint8_t mqtt_full_connect(void)
{
    if (!wifi_connect_only()) return 0;
    if (!mqtt_connect_only()) return 0;
    return 1;
}

void ESP8266_HandleCommand(const char *topic, const char *json)
{
    if (!topic || !json) return;

    if (strstr(topic, "attributes/push")) {

        float f = 0.0f;
        uint64_t ts = 0;
        uint8_t changed_threshold = 0;

        /* ? */
        if (json_get_float(json, "\"yu_tmp\"", &f))   { set_temp_threshold  = (uint8_t)f; changed_threshold = 1; }
        if (json_get_float(json, "\"yu_hum\"", &f))   { set_hum_threshold   = (uint8_t)f; changed_threshold = 1; }
        if (json_get_float(json, "\"yu_light\"", &f)) { set_light_threshold = (uint8_t)f; changed_threshold = 1; }

        /*  */
        if (json_get_float(json, "\"fan_state\"", &f)) {
            dev_fan_state = (uint8_t)f;
            Fan_set(dev_fan_state);
        }

        /*  */
        if (json_get_float(json, "\"curtain_state\"", &f)) {
            dev_curtain_state = (uint8_t)f;
            if (dev_curtain_state) {
                Servo_SetON;    /*  Servo.h ????{Servo_SetON;} */
            } else {
                Servo_SetOFF;
            }
        }

        /*  */
        if (json_get_float(json, "\"LED_state\"", &f)) {
            dev_led_level = (uint8_t)f;
            if (dev_led_level > 100) dev_led_level = 100;
            LED_Set_light(dev_led_level);
        }

        /* ßµ?ts(ms) */
        if (!g_time_synced && json_get_u64(json, "\"ts\"", &ts)) {
            RTC_SetCounter((uint32_t)(ts / 1000ULL));
            g_time_synced = 1;
        }

        /* ?Å£??*/
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

        /* ?°§ => ? */
        g_auto_mode = 0;
    }
}

/* ========== MQTT ¬÷—Ø£∫ESP8266_Process + Œ»∂®÷ÿ¡¨ ========== */
void EspMqtt_Task(void *arg)
{
    uint8_t wifi_lost_cnt = 0;
    uint8_t mqtt_lost_cnt = 0;
    uint8_t reconnect_fail_cnt = 0;
    TickType_t next_reconnect_tick = 0;
    uint32_t reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MIN_MS;

    (void)arg;

    ESP8266_SetMqttMsgCallback(on_mqtt_subrecv);
    if (mqtt_full_connect()) {
        next_reconnect_tick = xTaskGetTickCount();
    } else {
        next_reconnect_tick = xTaskGetTickCount() + pdMS_TO_TICKS(reconnect_backoff_ms);
    }

    while (1) {
        ESP8266_Process();

        if (!ESP8266_IsWiFiOK()) {
            if (wifi_lost_cnt < 255u) wifi_lost_cnt++;
            mqtt_lost_cnt = 0;
        } else {
            wifi_lost_cnt = 0;
            if (!ESP8266_IsMQTTOK()) {
                if (mqtt_lost_cnt < 255u) mqtt_lost_cnt++;
            } else {
                mqtt_lost_cnt = 0;
                reconnect_fail_cnt = 0;
                reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MIN_MS;
                next_reconnect_tick = xTaskGetTickCount();
            }
        }

        if ((wifi_lost_cnt >= WIFI_LOST_DEBOUNCE_COUNT) ||
            (ESP8266_IsWiFiOK() && mqtt_lost_cnt >= MQTT_LOST_DEBOUNCE_COUNT)) {

            TickType_t now = xTaskGetTickCount();
            if ((int32_t)(now - next_reconnect_tick) >= 0) {
                uint8_t ok;

                if (wifi_lost_cnt >= WIFI_LOST_DEBOUNCE_COUNT) {
                    ok = mqtt_full_connect();
                } else {
                    ok = mqtt_connect_only();
                }

                if (ok) {
                    wifi_lost_cnt = 0;
                    mqtt_lost_cnt = 0;
                    reconnect_fail_cnt = 0;
                    reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MIN_MS;
                    next_reconnect_tick = now;
                } else {
                    reconnect_fail_cnt++;
                    if (reconnect_backoff_ms < MQTT_RECONNECT_BACKOFF_MAX_MS) {
                        reconnect_backoff_ms += MQTT_RECONNECT_BACKOFF_MIN_MS;
                        if (reconnect_backoff_ms > MQTT_RECONNECT_BACKOFF_MAX_MS) {
                            reconnect_backoff_ms = MQTT_RECONNECT_BACKOFF_MAX_MS;
                        }
                    }
                    next_reconnect_tick = now + pdMS_TO_TICKS(reconnect_backoff_ms);

                    if (reconnect_fail_cnt >= MQTT_REINIT_FAIL_THRESHOLD) {
                        ESP8266_Init(115200);
                        reconnect_fail_cnt = 0;
                        wifi_lost_cnt = WIFI_LOST_DEBOUNCE_COUNT;
                        mqtt_lost_cnt = 0;
                    }
                }
            }
        }

        if (g_mqtt_pub_q) {
            mqtt_pub_msg_t m;
            if (xQueueReceive(g_mqtt_pub_q, &m, pdMS_TO_TICKS(100)) == pdPASS) {
                if (ESP8266_IsMQTTOK()) {
                    if (!ESP8266_MQTT_Publish(MQTT_LINK_ID,
                                              m.topic,
                                              m.payload,
                                              m.qos,
                                              m.retain,
                                              PUB_TIMEOUT_MS)) {
                        xQueueSendToFront(g_mqtt_pub_q, &m, 0);
                        mqtt_lost_cnt = MQTT_LOST_DEBOUNCE_COUNT;
                    }
                } else {
                    xQueueSendToFront(g_mqtt_pub_q, &m, 0);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
