#include "ESP8266.h"
#include <string.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "USART.h"

/* 是否开启详细 URC 调试（打印原始字节十六进制及时间戳） */
static volatile uint8_t s_debug_verbose = 0;

void ESP8266_SetDebug(uint8_t enable)
{
    s_debug_verbose = enable ? 1 : 0;
}

/* ===== 外部依赖 ===== */
extern void USART3_Init(unsigned int baudrate);
extern void USART3_SendString(char *str);

extern void delay_ms(unsigned int ms);
extern void Delay_ms(unsigned int ms);


static void esp_delay_ms(unsigned int ms_)
{
#ifdef FreeRTOS_h
    /* 如果已经启动调度器，用 vTaskDelay；否则用裸机 delay */
    if (xTaskGetTickCount() > 0) {
        vTaskDelay(pdMS_TO_TICKS(ms_));
        return;
    }
#endif
    delay_ms(ms_);
}



/* ================== Ring Buffer ================== */
static volatile uint16_t s_rx_w = 0;
static volatile uint16_t s_rx_r = 0;
static uint8_t s_rx_ring[ESP8266_RX_RING_SIZE];

/* 状态 */
static volatile uint8_t s_wifi_ok = 0;
static volatile uint8_t s_mqtt_ok = 0;

uint8_t ESP8266_IsWiFiOK(void) { return s_wifi_ok; }
uint8_t ESP8266_IsMQTTOK(void) { return s_mqtt_ok; }

static uint16_t rx_available(void)
{
    uint16_t w = s_rx_w, r = s_rx_r;
    if (w >= r) return (uint16_t)(w - r);
    return (uint16_t)(ESP8266_RX_RING_SIZE - (r - w));
}

static void rx_push(uint8_t b)
{
    uint16_t next = (uint16_t)(s_rx_w + 1u);
    if (next >= ESP8266_RX_RING_SIZE) next = 0;

    /* 满了就丢弃最旧 */
    if (next == s_rx_r)
    {
        uint16_t rnext = (uint16_t)(s_rx_r + 1u);
        if (rnext >= ESP8266_RX_RING_SIZE) rnext = 0;
        s_rx_r = rnext;
    }

    s_rx_ring[s_rx_w] = b;
    s_rx_w = next;
}

static void esp_rx_flush(void)
{
    s_rx_r = s_rx_w;
}

/* ================== USART3 IRQ ==================
 * 注意：ISR 里不要 printf/不要转发到 USART1
 */
void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        volatile uint16_t sr = USART3->SR;
        (void)sr;

        uint8_t b = (uint8_t)(USART3->DR & 0xFF);
        rx_push(b);

        /* ✅ 不要在中断里做串口转发/printf，容易丢数据 */
        // USART1_SendByte(b);

        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}


/* ================== 读字节：用于“全量打印” ================== */
uint8_t ESP8266_ReadByte(uint8_t *b)
{
    if (!b) return 0;
    if (s_rx_r == s_rx_w) return 0;

    *b = s_rx_ring[s_rx_r];
    s_rx_r++;
    if (s_rx_r >= ESP8266_RX_RING_SIZE) s_rx_r = 0;
    return 1;
}

/* ================== 读行（\n结束） ================== */
uint8_t ESP8266_ReadLine(char *out, uint16_t out_len)
{
    static char partial[256];
    static uint16_t p_len = 0;

    if (!out || out_len < 2) return 0;

    while (rx_available() > 0)
    {
        uint8_t b = s_rx_ring[s_rx_r];

        /* 消费字节 */
        s_rx_r++;
        if (s_rx_r >= ESP8266_RX_RING_SIZE) s_rx_r = 0;

        if (p_len >= sizeof(partial) - 1)
        {
            /* 太长就重置，避免卡死 */
            p_len = 0;
        }

        partial[p_len++] = (char)b;
        partial[p_len] = '\0';

        if (b == '\n') /* 只以 \n 为一行结束 */
        {
            /* 去掉末尾 \r\n */
            while (p_len > 0 &&
                   (partial[p_len - 1] == '\n' || partial[p_len - 1] == '\r'))
            {
                partial[p_len - 1] = '\0';
                p_len--;
            }

            /* 复制一份用于输出 */
            strncpy(out, partial, out_len - 1);
            out[out_len - 1] = '\0';

            /* 额外调试：打印原始字节（十六进制）和 RTOS tick 时间，便于诊断时序 */
            if (s_debug_verbose)
            {
                uint16_t i;
                /* 打印时间戳（tick） */
#ifdef FreeRTOS_h
                printf("[ESP-RAW][tick=%u] ", (unsigned)xTaskGetTickCount());
#else
                printf("[ESP-RAW] ");
#endif
                for (i = 0; i < p_len; i++)
                {
                    printf("%02X ", (unsigned char)partial[i]);
                }
                printf("\r\n[ESP-LINE] %s\r\n", partial);
            }

            p_len = 0;
            partial[0] = '\0';
            return 1;
        }
    }
    return 0;
}

/* ================== AT 发送/等待 ================== */
static void handle_urc_line(const char *line);


static void esp_send_cmd(const char *cmd)
{
    char buf[640];
    snprintf(buf, sizeof(buf), "%s\r\n", cmd);

    taskENTER_CRITICAL();
    USART3_SendString(buf);
    taskEXIT_CRITICAL();

    printf("[ESP-TX] %s\r\n", cmd);
}



/* 逐行等待 ack，并顺便更新状态 */
static uint8_t esp_wait_ack(const char *ack, uint32_t timeout_ms)
{
    char line[256];

    while (timeout_ms--)
    {
        while (ESP8266_ReadLine(line, (uint16_t)sizeof(line)))
        {
            if (line[0] == '\0') continue;

            printf("[ESP-CONSUME] %s\r\n", line);

            /* ✅ 关键：等待 ACK 时也要处理 URC（包括 +MQTTSUBRECV） */
            handle_urc_line(line);

            /* ❗可选：如果你仍担心 handle_urc_line 没覆盖到某些状态，就保留下面这几行
               但一般可以删掉，因为 handle_urc_line 已经做了 */
#if 0
            if (strstr(line, "WIFI GOT IP"))         s_wifi_ok = 1;
            if (strstr(line, "WIFI DISCONNECT"))     { s_wifi_ok = 0; s_mqtt_ok = 0; }
            if (strstr(line, "+MQTTCONNECTED:"))     s_mqtt_ok = 1;
            if (strstr(line, "+MQTTDISCONNECTED:"))  s_mqtt_ok = 0;
#endif

            if (strstr(line, "ERROR")) return 0;
            if (strstr(line, "FAIL"))  return 0;
            if (ack && strstr(line, ack)) return 1;
        }
        esp_delay_ms(1);
    }
    return 0;
}



static uint8_t esp_cmd_ack_retry(const char *cmd, const char *ack,
                                uint32_t timeout_ms, uint8_t retry)
{
    static SemaphoreHandle_t s_tx_mutex = NULL;
    uint8_t locked = 0;

    /* 确保在有调度器时创建互斥锁以序列化对串口的写入，避免命令交错 */
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        if (s_tx_mutex == NULL)
        {
            s_tx_mutex = xSemaphoreCreateMutex();
        }
        if (s_tx_mutex)
        {
            /* 等待直到获得互斥锁，序列化发送 */
            xSemaphoreTake(s_tx_mutex, portMAX_DELAY);
            locked = 1;
        }
    }

    for (uint8_t i = 0; i <= retry; i++)
    {
        esp_send_cmd(cmd);
        if (esp_wait_ack(ack, timeout_ms))
        {
            /* ✅ 成功也必须释放 mutex */
            if (locked && s_tx_mutex)
            {
                xSemaphoreGive(s_tx_mutex);
            }
            return 1;
        }
    }

    /* 释放互斥锁（如果已获得） */
    if (locked && s_tx_mutex)
    {
        xSemaphoreGive(s_tx_mutex);
    }
    return 0;
}

static uint8_t esp_wait_prompt(uint32_t timeout_ms)
{
    uint8_t b;

    while (timeout_ms--)
    {
        while (ESP8266_ReadByte(&b))
        {
            /* 兼容：有的固件 prompt 是单独一个 '>'，不带换行 */
            if (b == '>') return 1;

            /* 顺手把 ERROR/FAIL 也拦一下（按字节简单检测，保险） */
            /* 这里不做复杂状态机，真正的 FAIL/ERROR 仍由后续 esp_wait_ack 处理 */
        }
        esp_delay_ms(1);
    }
    return 0;
}




/* ================== 对外 API ================== */
void ESP8266_Init(uint32_t baud)
{
    USART3_Init((unsigned int)baud);

    esp_delay_ms(300);
    esp_rx_flush();

    /* 软复位同步状态 */
    esp_send_cmd("AT+RST");
    (void)esp_wait_ack("ready", 3000);
    esp_delay_ms(200);
    esp_rx_flush();

    (void)esp_cmd_ack_retry("AT", "OK", 1000, 2);
    (void)esp_cmd_ack_retry("ATE0", "OK", 1000, 2);
    (void)esp_cmd_ack_retry("AT+CWMODE=1", "OK", 1000, 2);

    s_wifi_ok = 0;
    s_mqtt_ok = 0;
}

uint8_t ESP8266_WiFi_Connect(const wifi_t *wifi, uint32_t timeout_ms, uint8_t retry)
{
    char cmd[200];
    if (!wifi || !wifi->ssid || !wifi->password) return 0;

    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", wifi->ssid, wifi->password);
    return esp_cmd_ack_retry(cmd, "OK", timeout_ms, retry);
}

uint8_t ESP8266_WiFi_Disconnect(uint32_t timeout_ms)
{
    return esp_cmd_ack_retry("AT+CWQAP", "OK", timeout_ms, 0);
}

uint8_t ESP8266_MQTT_UserCfg(const mqtt_usercfg_t *cfg, uint32_t timeout_ms)
{
    char cmd[520];
    if (!cfg || !cfg->client_id || !cfg->username || !cfg->password) return 0;

    snprintf(cmd, sizeof(cmd),
             "AT+MQTTUSERCFG=%u,%u,\"%s\",\"%s\",\"%s\",0,0,\"\"",
             (unsigned)cfg->link_id,
             (unsigned)cfg->scheme,
             cfg->client_id,
             cfg->username,
             cfg->password);

    return esp_cmd_ack_retry(cmd, "OK", timeout_ms, ESP8266_AT_DEFAULT_RETRY);
}

uint8_t ESP8266_MQTT_Connect(uint8_t link_id, const char *host, uint16_t port,
                            uint8_t reconnect, uint32_t timeout_ms, uint8_t retry)
{
    char cmd[260];
    if (!host) return 0;

    snprintf(cmd, sizeof(cmd),
             "AT+MQTTCONN=%u,\"%s\",%u,%u",
             (unsigned)link_id,
             host,
             (unsigned)port,
             (unsigned)reconnect);

    return esp_cmd_ack_retry(cmd, "OK", timeout_ms, retry);
}

uint8_t ESP8266_MQTT_Subscribe(uint8_t link_id, const char *topic, uint8_t qos,
                              uint32_t timeout_ms)
{
    char cmd[360];
    if (!topic) return 0;

    snprintf(cmd, sizeof(cmd),
             "AT+MQTTSUB=%u,\"%s\",%u",
             (unsigned)link_id, topic, (unsigned)qos);

    return esp_cmd_ack_retry(cmd, "OK", timeout_ms, ESP8266_AT_DEFAULT_RETRY);
}

uint8_t ESP8266_MQTT_Publish(uint8_t link_id, const char *topic, const char *payload,
                            uint8_t qos, uint8_t retain, uint32_t timeout_ms)
{
    if (!topic || !payload) return 0;

    uint32_t len = (uint32_t)strlen(payload);

    /* ✅ 正确顺序：<length>,<qos>,<retain> */
    char cmd[420];
    snprintf(cmd, sizeof(cmd),
             "AT+MQTTPUBRAW=%u,\"%s\",%lu,%u,%u",
             (unsigned)link_id,
             topic,
             (unsigned long)len,
             (unsigned)qos,
             (unsigned)retain);

    /* 1) 发命令，先等 OK（按文档先回 OK 再给 '>'） */
    if (!esp_cmd_ack_retry(cmd, "OK", timeout_ms, 0))
        return 0;

    /* 2) 等待 '>' prompt（注意：不靠 ReadLine） */
    if (!esp_wait_prompt(timeout_ms))
        return 0;

    /* 3) 发 payload 原始数据：不加 \r\n，长度以 len 为准 */
    taskENTER_CRITICAL();
    USART3_SendString((char *)payload);
    taskEXIT_CRITICAL();

    /* 4) 等待发布结果：成功是 +MQTTPUB:OK，失败是 +MQTTPUB:FAIL */
    if (!esp_wait_ack("+MQTTPUB:OK", timeout_ms))
        return 0;

    return 1;
}



uint8_t ESP8266_MQTT_Clean(uint8_t link_id, uint32_t timeout_ms)
{
    char cmd[64];

    snprintf(cmd, sizeof(cmd), "AT+MQTTCLEAN=%u", (unsigned)link_id);

    if (esp_cmd_ack_retry(cmd, "OK", timeout_ms, 0))
    {
        s_mqtt_ok = 0;
        return 1;
    }

    /* 某些固件在已断开时会直接 ERROR，这里至少把本地状态清掉 */
    s_mqtt_ok = 0;
    return 0;
}





/* ================== 订阅回调（可选） ================== */
static esp8266_mqtt_msg_cb_t s_mqtt_cb = 0;
static char s_topic_buf[192];
static char s_payload_buf[256];

void ESP8266_SetMqttMsgCallback(esp8266_mqtt_msg_cb_t cb)
{
    s_mqtt_cb = cb;
    printf("[ESP] mqtt_cb=%p\r\n", (void*)cb);
}


/* ====== 新增：解析无引号 payload 的 +MQTTSUBRECV ====== */
static uint8_t parse_uint_field(const char *s, uint32_t *idx, uint32_t *out)
{
    uint32_t v = 0;
    const char *p;

    if (!s || !idx || !out) return 0;
    p = s + *idx;

    while (*p == ' ' || *p == '\t') p++;
    if (*p < '0' || *p > '9') return 0;

    while (*p >= '0' && *p <= '9')
    {
        v = v * 10u + (uint32_t)(*p - '0');
        p++;
    }

    *out = v;
    *idx = (uint32_t)(p - s);
    return 1;
}

/* 解析 "xxx" 形式的字段 */
static uint8_t parse_quoted_field(const char *s,
                                  uint32_t *idx,
                                  char *out,
                                  uint32_t out_sz)
{
    uint32_t i = 0;

    if (!s || !idx || !out || out_sz < 2) return 0;

    /* 跳过空格 */
    while (s[*idx] == ' ' || s[*idx] == '\t') (*idx)++;

    /* 必须以 " 开始 */
    if (s[*idx] != '\"') return 0;
    (*idx)++;  /* 跳过开头的 " */

    while (s[*idx] != '\0' && s[*idx] != '\"')
    {
        if (i < out_sz - 1)
        {
            out[i++] = s[*idx];
        }
        (*idx)++;
    }

    if (s[*idx] != '\"') return 0;

    out[i] = '\0';

    (*idx)++;  /* 跳过结尾的 " */

    return 1;
}


/* 原来的 parse_quoted_field 你可以保留不动，这里直接复用 */
static uint8_t parse_mqttsubrecv_line(const char *line,
                                     uint8_t *o_link_id,
                                     char *topic, uint32_t topic_sz,
                                     char *payload, uint32_t payload_sz)
{
    const char *p;
    uint32_t idx;
    uint32_t link_id = 0;
    uint32_t data_len = 0;

    if (!line || !o_link_id || !topic || !payload) return 0;
    if (topic_sz < 2 || payload_sz < 2) return 0;

    p = strstr(line, "+MQTTSUBRECV:");
    if (!p) return 0;
    p += strlen("+MQTTSUBRECV:");

    /* link_id */
    while (*p == ' ' || *p == '\t') p++;
    if (*p < '0' || *p > '9') return 0;

    while (*p >= '0' && *p <= '9')
    {
        link_id = link_id * 10u + (uint32_t)(*p - '0');
        p++;
    }
    *o_link_id = (uint8_t)link_id;

    idx = (uint32_t)(p - line);

    /* ✅ 关键：跳过 link_id 后面的逗号 */
    while (line[idx] == ' ' || line[idx] == '\t') idx++;
    if (line[idx] != ',') return 0;
    idx++; /* skip ',' */

    /* topic 必须是引号包起来的 */
    topic[0] = '\0';
    payload[0] = '\0';
    if (!parse_quoted_field(line, &idx, topic, topic_sz))
        return 0;

    /* 跳到下一个字段（topic 后面的逗号） */
    while (line[idx] == ' ' || line[idx] == '\t') idx++;
    if (line[idx] != ',') return 0;
    idx++; /* skip ',' */

    while (line[idx] == ' ' || line[idx] == '\t') idx++;

    /* 两种格式：
       A) "payload"
       B) len,payload(无引号，可能含逗号/引号/大括号) */
    if (line[idx] == '\"')
    {
        if (!parse_quoted_field(line, &idx, payload, payload_sz))
            return 0;
        return 1;
    }
    else
    {
        /* B) 新格式：先是长度，再逗号，再是 payload 原文 */
        if (!parse_uint_field(line, &idx, &data_len))
        {
            data_len = 0;
        }

        while (line[idx] == ' ' || line[idx] == '\t') idx++;
        if (line[idx] == ',') idx++;   /* skip ',' after len */
        while (line[idx] == ' ' || line[idx] == '\t') idx++;

        /* payload 从 idx 开始到行末 */
        {
            const char *pay = line + idx;
            uint32_t pay_real_len = (uint32_t)strlen(pay);
            uint32_t n = pay_real_len;

            if (data_len > 0 && data_len <= pay_real_len) n = data_len;
            if (n >= payload_sz) n = payload_sz - 1;

            memcpy(payload, pay, n);
            payload[n] = '\0';
        }
        return 1;
    }
}


/* ====== 修改：handle_urc_line()，用新的解析器 ====== */
static void handle_urc_line(const char *line)
{
    if (!line) return;

    /* 状态 */
    if (strstr(line, "WIFI GOT IP"))        { s_wifi_ok = 1; return; }
    if (strstr(line, "WIFI DISCONNECT"))    { s_wifi_ok = 0; s_mqtt_ok = 0; return; }
    if (strstr(line, "+MQTTCONNECTED:"))    { s_mqtt_ok = 1; return; }
    if (strstr(line, "+MQTTDISCONNECTED:")) { s_mqtt_ok = 0; return; }

    /* 订阅消息：兼容两种 +MQTTSUBRECV 格式 */
    if (strstr(line, "+MQTTSUBRECV:"))
	{
		uint8_t link_id = 0;

		printf("[ESP] hit SUBRECV\r\n");

		s_topic_buf[0] = '\0';
		s_payload_buf[0] = '\0';

		if (!parse_mqttsubrecv_line(line,
									&link_id,
									s_topic_buf, sizeof(s_topic_buf),
									s_payload_buf, sizeof(s_payload_buf)))
		{
			printf("[ESP] parse SUBRECV fail\r\n");
			return;
		}

		printf("[ESP] sub topic=%s\r\n", s_topic_buf);
		printf("[ESP] sub payload=%s\r\n", s_payload_buf);
		printf("[ESP] cb=%p\r\n", (void*)s_mqtt_cb);

		if (s_mqtt_cb)
		{
			s_mqtt_cb(link_id, s_topic_buf, s_payload_buf);
		}
		return;
	}

}


/* 可选：消费 ring 并解析 URC（不需要解析时可以不调用） */
void ESP8266_Process(void)
{
    char line[256];
    while (ESP8266_ReadLine(line, (uint16_t)sizeof(line)))
    {
        handle_urc_line(line);
    }
}
