#ifndef __ESP8266_H
#define __ESP8266_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ================= 配置 ================= */
#ifndef ESP8266_RX_RING_SIZE
#define ESP8266_RX_RING_SIZE 2048
#endif

#ifndef ESP8266_AT_DEFAULT_RETRY
#define ESP8266_AT_DEFAULT_RETRY 2
#endif

/* ================= 类型 ================= */
typedef struct {
    const char *ssid;
    const char *password;
} wifi_t;

typedef struct {
    uint8_t link_id;     /* 0..4 */
    uint8_t scheme;      /* 1=TCP (明文 1883) */
    const char *client_id;
    const char *username;
    const char *password;
} mqtt_usercfg_t;

/* 订阅回调（可选） */
typedef void (*esp8266_mqtt_msg_cb_t)(uint8_t link_id, const char *topic, const char *payload);


/* ================= API ================= */
void ESP8266_Init(uint32_t baud);

/* WiFi */
uint8_t ESP8266_WiFi_Connect(const wifi_t *wifi, uint32_t timeout_ms, uint8_t retry);
uint8_t ESP8266_WiFi_Disconnect(uint32_t timeout_ms);

/* MQTT */
uint8_t ESP8266_MQTT_UserCfg(const mqtt_usercfg_t *cfg, uint32_t timeout_ms);
uint8_t ESP8266_MQTT_Connect(uint8_t link_id, const char *host, uint16_t port,
                            uint8_t reconnect, uint32_t timeout_ms, uint8_t retry);
uint8_t ESP8266_MQTT_Subscribe(uint8_t link_id, const char *topic, uint8_t qos,
                              uint32_t timeout_ms);
uint8_t ESP8266_MQTT_Publish(uint8_t link_id, const char *topic, const char *payload,
                            uint8_t qos, uint8_t retain, uint32_t timeout_ms);
uint8_t ESP8266_MQTT_Clean(uint8_t link_id, uint32_t timeout_ms);

/* 接收 */
uint8_t ESP8266_ReadLine(char *out, uint16_t out_len);   /* 按行读（以 \n 为一行结束） */
uint8_t ESP8266_ReadByte(uint8_t *b);                    /* 字节读（用于全量打印） */

void ESP8266_SetMqttMsgCallback(esp8266_mqtt_msg_cb_t cb);


/* Enable/disable verbose URC debug prints from ESP8266_ReadLine/Process */
void ESP8266_SetDebug(uint8_t enable);
void ESP8266_Process(void);                               /* 可选：解析URC、更新状态 */

uint8_t ESP8266_IsWiFiOK(void);
uint8_t ESP8266_IsMQTTOK(void);

#ifdef __cplusplus
}
#endif

#endif
