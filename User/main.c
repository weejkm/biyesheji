#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "USART.h"
#include "delay.h"
#include "OLED.h"
#include "Key.h"
#include "light.h"
#include "MyRTC.h"
#include "esp8266.h"
#include "LED.h"
#include "Encoder.h"
#include "rc522.h"
#include "Servo.h"
#include "SU.h"
#include "Fan.h"

#include "app_state.h"
#include "mqtt_task.h"
#include "detection_task.h"
#include "key_task.h"
#include "oled_task.h"

/* 系统初始化函数 */
void sys_init(void);

/* 开机启动页面与模式选择函数 */
void start_page(void);

/* =========================================================================
 * 主函数
 *
 * 功能说明：
 * 1. 完成底层硬件初始化；
 * 2. 显示开机启动页面并选择工作模式；
 * 3. 创建系统各个 FreeRTOS 任务；
 * 4. 创建 MQTT 发布消息队列；
 * 5. 启动 FreeRTOS 调度器。
 * ========================================================================= */
int main(void)
{
    sys_init();
    start_page();
	
    /* ===================== 创建系统任务 ===================== */
    xTaskCreate(OLED_Task,      "OLED",   512, NULL, 4, NULL);
    xTaskCreate(Detection_Task, "Detect", 256, NULL, 2, NULL);
    xTaskCreate(KEY_Task,       "KEY",    512, NULL, 3, NULL);

    /* 创建 MQTT 发布消息队列 */
    g_mqtt_pub_q = xQueueCreate(8, sizeof(mqtt_pub_msg_t));

    /* 创建 MQTT / WiFi 通信任务 */
    xTaskCreate(EspMqtt_Task, "MQTT", 768, NULL, 1, NULL);

    /* 启动任务调度器 */
    vTaskStartScheduler();

    /* 正常情况下不会运行到这里 */
}

/* =========================================================================
 * 系统初始化函数
 *
 * 功能说明：
 * 按照系统运行所需顺序，依次初始化各个外设和功能模块。
 *
 * 初始化内容包括：
 * 1. 延时模块
 * 2. 串口调试
 * 3. OLED 显示
 * 4. 按键输入
 * 5. ADC 光照采集
 * 6. RTC 实时时钟
 * 7. ESP8266 WiFi 模块
 * 8. LED
 * 9. 旋转编码器
 * 10. RC522 RFID 模块
 * 11. 舵机
 * 12. 语音识别模块
 * 13. 风扇
 * 14. DHT11 温湿度模块
 * ========================================================================= */
void sys_init(void)
{
    delay_init();

    /* 初始化串口1，用于打印调试信息 */
    USART1_Init(115200);

    /* 初始化 OLED，并显示启动等待提示 */
    OLED_Init();
    OLED_Clear();
    OLED_ShowString(16, 16, "please wait", OLED_8X16);
    OLED_Update();

    /* 初始化按键模块 */
    Key_Init();

    /* 初始化 ADC，用于光照采样 */
    Init_ADC1();

    /* 初始化 RTC 实时时钟 */
    MyRTC_Init();

    /* 初始化 ESP8266 模块 */
    ESP8266_Init(115200);

    /* 初始化 LED 控制 */
    LED_Init();

    /* 初始化旋转编码器 */
    Encoder_Init();

    /* 初始化 RC522 RFID 读卡器 */
    RC522_SPI_Init();

    /* 初始化舵机 */
    Servo_Init();

    /* 初始化语音识别模块 */
    SU_Init();

    /* 初始化风扇控制 */
    Fan_Init();

    /* 初始化 DHT11 温湿度传感器 */
    DHT11_Init();
}

/* =========================================================================
 * 开机页面与模式选择函数
 *
 * 功能说明：
 * 1. 开机后在 OLED 上显示模式选择提示；
 * 2. 用户可通过按键选择在线模式或离线模式；
 * 3. 若在限定时间内没有按键操作，则自动进入默认模式。
 *
 * 当前逻辑：
 * - KEY1：进入默认模式（通常为在线模式，当前代码中未显式赋值）
 * - KEY2：进入离线模式（g_online_mode = 0）
 * - 若超时无操作，则直接进入默认模式
 *
 * 说明：
 * 当前 OLED 中文显示的乱码，是因为原始中文注释或字模文本编码异常。
 * 如果字库和编码匹配正确，可替换为正常中文内容。
 * ========================================================================= */
void start_page(void)
{
    OLED_Clear();
    OLED_Update();

    /* 显示模式选择界面 */
    OLED_ShowChinese(30, 0, "选择模式");
    OLED_ShowString(16, 16, "KEY1", OLED_8X16);
    OLED_ShowChinese(48, 16, "在线");
    OLED_ShowString(16, 32, "KEY2", OLED_8X16);
    OLED_ShowChinese(48, 32, "离线");
    OLED_Update();

    uint8_t OutTime = 0;

    while (1)
    {
        if (Key_GetState(KEY1) == CLICK)
        {
            /*
             * KEY1：选择默认模式
             * 如果需要明确指定为在线模式，可取消下行注释
             */
            // g_online_mode = 1;
            break;
        }
        else if (Key_GetState(KEY2) == CLICK)
        {
            /*
             * KEY2：选择离线模式
             */
            g_online_mode = 0;
            break;
        }

        Delay_ms(50);

        /*
         * 超时退出：
         * 每次循环延时 50ms，
         * 100 次约等于 5 秒。
         * 若用户 5 秒内未选择，则自动进入默认模式。
         */
        if (++OutTime >= 100)
            break;
    }

    /* 退出模式选择页面，显示等待提示 */
    OLED_Clear();
    OLED_ShowString(16, 16, "please wait", OLED_8X16);
    OLED_Update();
}
