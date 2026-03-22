#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#include "OLED.h"
#include "OLED_Data.h"
#include "MyRTC.h"
#include "ESP8266.h"

#include "app_state.h"

/* RTC 时间数组，由 MyRTC 模块提供 */
extern uint16_t MyRTC_Time[];

/* =========================================================================
 * OLED 显示任务
 *
 * 功能说明：
 * 1. 周期性刷新 OLED 显示内容；
 * 2. 根据 g_oled_page 显示不同页面：
 *    - 页面0：主页面，显示时间、温度、湿度、光照
 *    - 页面1：设备控制页面，显示窗帘、风扇、灯光状态
 *    - 页面2：阈值设置页面，显示温度/湿度/光照阈值
 *    - 页面3：RFID 存储提示页面
 * 3. 根据网络状态显示 WiFi / MQTT 联网图标；
 * 4. 刷新周期为 100ms。
 * ========================================================================= */
void OLED_Task(void *arg)
{
    uint8_t buf[32] = {0};
    (void)arg;

    for (;;)
    {
        /* 每次刷新前先清屏 */
        OLED_Clear();

        /* ===================== 页面0：主显示页面 ===================== */
        if (g_oled_page == 0)
        {
            /* 读取 RTC 时间并显示当前时分秒 */
            MyRTC_ReadTime();
            snprintf((char *)buf, sizeof(buf), "%02d:%02d:%02d",
                     MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
            OLED_ShowString(32, 0, (char *)buf, OLED_8X16);

            /* 显示当前温度 */
            OLED_ShowChinese(8, 16, "温度：");
            snprintf((char *)buf, sizeof(buf), "%d.%d C",
                     dev_dht11.temperature_int, dev_dht11.temperature_dec);
            OLED_ShowString(64, 16, (char *)buf, OLED_8X16);

            /* 显示当前湿度 */
            OLED_ShowChinese(8, 32, "湿度：");
            snprintf((char *)buf, sizeof(buf), "%d.%d %%",
                     dev_dht11.humidity_int, dev_dht11.humidity_dec);
            OLED_ShowString(64, 32, (char *)buf, OLED_8X16);

            /* 显示当前光照百分比 */
            OLED_ShowChinese(8, 48, "光照：");
            snprintf((char *)buf, sizeof(buf), "%.1f %%",
                     dev_light_percent);
            OLED_ShowString(64, 48, (char *)buf, OLED_8X16);
        }

        /* ===================== 页面1：设备控制页面 ===================== */
        else if (g_oled_page == 1)
        {
            /* 页面标题 */
            OLED_ShowChinese(34, 0, "设备控制");

            /* 显示当前选中项的箭头 */
            OLED_ShowImage(0, 16 * (g_oled_select + 1), 16, 16, jiantou);

            /* 显示窗帘状态 */
            OLED_ShowChinese(16, 16, "窗帘：");
            if (dev_curtain_state == CURTAIN_OFF)
                OLED_ShowChinese(64, 16, "关闭");
            else
                OLED_ShowChinese(64, 16, "打开");

            /* 显示风扇状态 */
            OLED_ShowChinese(16, 32, "风扇：");
            if (dev_fan_state == FAN_OFF)
                OLED_ShowChinese(64, 32, "关闭");
            else
                OLED_ShowChinese(64, 32, "打开");

            /* 显示灯光亮度 */
            OLED_ShowChinese(16, 48, "灯光：");
            OLED_ShowNum(64, 48, dev_led_level, 3, OLED_8X16);
        }

        /* ===================== 页面2：阈值设置页面 ===================== */
        else if (g_oled_page == 2)
        {
            /* 页面标题 */
            OLED_ShowChinese(28, 0, "舒适度设置");

            /* 显示当前选中项的箭头 */
            OLED_ShowImage(0, 16 * (g_oled_select + 1), 16, 16, jiantou);

            /* 显示温度阈值 */
            OLED_ShowChinese(16, 16, "温度：");
            snprintf((char *)buf, sizeof(buf), "%d", set_temp_threshold);
            OLED_ShowString(64, 16, (char *)buf, OLED_8X16);

            /* 显示湿度阈值 */
            OLED_ShowChinese(16, 32, "湿度：");
            snprintf((char *)buf, sizeof(buf), "%d", set_hum_threshold);
            OLED_ShowString(64, 32, (char *)buf, OLED_8X16);

            /* 显示光照阈值 */
            OLED_ShowChinese(16, 48, "光照：");
            snprintf((char *)buf, sizeof(buf), "%d", set_light_threshold);
            OLED_ShowString(64, 48, (char *)buf, OLED_8X16);
        }

        /* ===================== 页面3：RFID 保存页面 ===================== */
        else if (g_oled_page == 3)
        {
            OLED_ShowChinese(26, 0, "保存到卡片");
            OLED_ShowString(16, 16, "KEY1:ESC", OLED_8X16);
            OLED_ShowString(16, 32, "KEY2:STORE", OLED_8X16);
        }

        /* ===================== 页面异常处理 ===================== */
        else
        {
            OLED_ShowString(16, 32, "ERROR!!!", OLED_8X16);
        }

        /* ===================== 网络状态显示 ===================== */
        if (g_online_mode)
        {
            /*
             * 在线模式下：
             * 只有当 WiFi 和 MQTT 都连接成功时，显示在线图标；
             * 否则显示离线图标，并提示正在重连。
             */
            if (ESP8266_IsWiFiOK() && ESP8266_IsMQTTOK())
            {
                OLED_ShowImage(112, 0, 16, 16, wifi_online);
            }
            else
            {
                OLED_ShowImage(112, 0, 16, 16, wifi_unonline);
                OLED_ShowString(0, 0, "RECONN", OLED_6X8);
            }
        }
        else
        {
            /*
             * 离线模式下直接显示离线图标
             */
            OLED_ShowImage(112, 0, 16, 16, wifi_unonline);
        }

        /* 刷新 OLED 显示缓存到屏幕 */
        OLED_Update();

        /* 任务刷新周期：100ms */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
