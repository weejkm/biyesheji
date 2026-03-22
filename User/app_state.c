#include "app_state.h"

/* ====================== OLED pages ====================== */
uint8_t g_oled_page   = 0;
uint8_t g_oled_select = 0;

/* ====================== device state / sensor / mode ====================== */
uint8_t dev_curtain_state = CURTAIN_OFF;
uint8_t dev_fan_state     = FAN_OFF;
uint8_t dev_led_level     = 0;

DHT11_Data_t dev_dht11 = {0};
float        dev_light_percent = 0.0f;

uint8_t set_temp_threshold  = 20;
uint8_t set_hum_threshold   = 80;
uint8_t set_light_threshold = 50;

uint8_t g_time_synced = 0;

uint8_t g_online_mode = 1;
uint8_t g_auto_mode   = 1;

/* ====================== MQTT publish queue ====================== */
QueueHandle_t g_mqtt_pub_q = NULL;

/* ====================== RFID ====================== */
uint8_t g_card_id[4];
uint8_t g_read_buf[16];
uint8_t g_write_buf[16] = {
    0x11, 0x22, 0x33, 0x44,
    0x55, 0x66, 0x77, 0x88,
    0x99, 0xAA, 0xBB, 0xCC,
    0xDD, 0xEE, 0xFF, 0x88
};

