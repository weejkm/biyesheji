#ifndef __MQTT_TASK_H
#define __MQTT_TASK_H

#include <stdint.h>
#include "app_state.h"   // mqtt_pub_msg_t、g_mqtt_pub_q、各种全局状态都在这里

/* MQTT task entry */
void EspMqtt_Task(void *arg);

/* Handle cloud push command */
void ESP8266_HandleCommand(const char *topic, const char *json);

#endif
