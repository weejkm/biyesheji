#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>

#include "light.h"
#include "dht11.h"

#include "app_state.h"

void Detection_Task(void *pvParameters)
{
    (void)pvParameters;

    TickType_t last_send_tick = 0;

    for (;;)
    {
        vTaskSuspendAll();
        dev_light_percent = (4096 - Read_ADC_Value()) * 1.0f / 4096.0f * 100.0f;
        DHT11_Read_Data(&dev_dht11);
		xTaskResumeAll();

        if (g_online_mode && g_mqtt_pub_q)//判断模式和消息队列有没有传数据过来
        {
            TickType_t now = xTaskGetTickCount();
            if ((now - last_send_tick) >= pdMS_TO_TICKS(10000))
            {
                last_send_tick = now;

                mqtt_pub_msg_t m;
                memset(&m, 0, sizeof(m));
                strcpy(m.topic, MQTT_TOPIC_PUB);
                m.qos = 0;
                m.retain = 0;

                snprintf(m.payload, sizeof(m.payload),
                         "{"
                         "\"tmp\":%.1f,"
                         "\"hum\":%.1f,"
                         "\"luminosity\":%.1f,"
                         "\"fan_state\":%d,"
                         "\"curtain_state\":%d,"
                         "\"LED_state\":%d,"
                         "\"yu_tmp\":%d,"
						 "\"yu_hum\":%d,"
                         "\"yu_light\":%.1f,"
                         "\"auto_mode\":%d"
                         "}",
                         (float)dev_dht11.temperature_int + (float)dev_dht11.temperature_dec * 0.1f,
                         (float)dev_dht11.humidity_int + (float)dev_dht11.humidity_dec * 0.1f,
                         (float)dev_light_percent,
                         (int)dev_fan_state,
                         (int)dev_curtain_state,
                         (int)dev_led_level,
                         (int)set_temp_threshold,
						 (int)set_hum_threshold,
                         (float)set_light_threshold,
                         (int)g_auto_mode);

                (void)xQueueSend(g_mqtt_pub_q, &m, 0);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
