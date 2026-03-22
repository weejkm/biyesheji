#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "dht11.h"

/* ====================== MQTT/WiFi (ThingsCloud) ====================== */
#define WIFI_SSID      "bin"
#define WIFI_PASS      "88888888"

#define MQTT_HOST      "sh-5-mqtt.iot-api.com"
#define MQTT_PORT      1883

#define MQTT_CLIENT_ID "ESP01"
#define MQTT_USERNAME  "5o6nhxbmqk80h82d"
#define MQTT_PASSWORD  "sIxS9ob7Qm"

/* ThingsCloud topics */
#define MQTT_TOPIC_PUB        "attributes"
#define MQTT_TOPIC_SUB_PUSH   "attributes/push"
#define MQTT_TOPIC_SUB_RESP   "attributes/response"
/* ===================================================================== */

/* ====================== OLED pages ====================== */
#define OLED_PAGE_COUNT 4

extern uint8_t g_oled_page;     /* 0..3 */
extern uint8_t g_oled_select;   /* 0..2 */
/* ======================================================= */

/* ====================== device state / sensor / mode ====================== */
#define CURTAIN_OFF 0
#define CURTAIN_ON  1
#define FAN_OFF     0
#define FAN_ON      1

extern uint8_t dev_curtain_state;
extern uint8_t dev_fan_state;
extern uint8_t dev_led_level;        /* 0..100 */

extern DHT11_Data_t dev_dht11;
extern float        dev_light_percent; /* 0..100 */

/* thresholds (can be set locally, from cloud, or via RFID) */
extern uint8_t set_temp_threshold;
extern uint8_t set_hum_threshold;
extern uint8_t set_light_threshold;

extern uint8_t g_time_synced;

/* modes */
extern uint8_t g_online_mode;   /* 1 online(MQTT) / 0 offline */
extern uint8_t g_auto_mode;     /* 1 auto / 0 manual */
/* ======================================================================== */

/* ====================== MQTT publish queue ====================== */
typedef struct {
    char topic[128];
    char payload[256];
    uint8_t qos;
    uint8_t retain;
} mqtt_pub_msg_t;

extern QueueHandle_t g_mqtt_pub_q;
/* ================================================================ */

/* RFID buffers (used by KEY task) */
extern uint8_t g_card_id[4];
extern uint8_t g_read_buf[16];
extern uint8_t g_write_buf[16];


#endif /* APP_STATE_H */
