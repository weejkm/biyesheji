#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#include "OLED.h"
#include "OLED_Data.h"
#include "MyRTC.h"

#include "app_state.h"

extern uint16_t MyRTC_Time[];

void OLED_Task(void *arg)
{
    uint8_t buf[32] = {0};
    (void)arg;

    for (;;)
    {
        OLED_Clear();

        if (g_oled_page == 0)
        {
            MyRTC_ReadTime();
            snprintf((char *)buf, sizeof(buf), "%02d:%02d:%02d", MyRTC_Time[3], MyRTC_Time[4], MyRTC_Time[5]);
            OLED_ShowString(32, 0, (char *)buf, OLED_8X16);

            OLED_ShowChinese(8, 16, "温度：");
            snprintf((char *)buf, sizeof(buf), "%d.%d C", dev_dht11.temperature_int, dev_dht11.temperature_dec);
            OLED_ShowString(64, 16, (char *)buf, OLED_8X16);

            OLED_ShowChinese(8, 32, "湿度：");
            snprintf((char *)buf, sizeof(buf), "%d.%d %%", dev_dht11.humidity_int, dev_dht11.humidity_dec);
            OLED_ShowString(64, 32, (char *)buf, OLED_8X16);

            OLED_ShowChinese(8, 48, "光照：");
            snprintf((char *)buf, sizeof(buf), "%.1f %%", dev_light_percent);
            OLED_ShowString(64, 48, (char *)buf, OLED_8X16);
        }
        else if (g_oled_page == 1)
        {
            OLED_ShowChinese(34, 0, "设备状态");
            OLED_ShowImage(0, 16 * (g_oled_select + 1), 16, 16, jiantou);

            OLED_ShowChinese(16, 16, "窗帘：");
            if (dev_curtain_state == CURTAIN_OFF) OLED_ShowChinese(64, 16, "关闭");
            else OLED_ShowChinese(64, 16, "打开");

            OLED_ShowChinese(16, 32, "风扇：");
            if (dev_fan_state == FAN_OFF) OLED_ShowChinese(64, 32, "关闭");
            else OLED_ShowChinese(64, 32, "打开");

            OLED_ShowChinese(16, 48, "灯光：");
            OLED_ShowNum(64, 48, dev_led_level, 3, OLED_8X16);
        }
        else if (g_oled_page == 2)
        {
            OLED_ShowChinese(34, 0, "目标环境");
            OLED_ShowImage(0, 16 * (g_oled_select + 1), 16, 16, jiantou);

            OLED_ShowChinese(16, 16, "温度：");
            snprintf((char *)buf, sizeof(buf), "%d", set_temp_threshold);
            OLED_ShowString(64, 16, (char *)buf, OLED_8X16);

            OLED_ShowChinese(16, 32, "湿度：");
            snprintf((char *)buf, sizeof(buf), "%d", set_hum_threshold);
            OLED_ShowString(64, 32, (char *)buf, OLED_8X16);

            OLED_ShowChinese(16, 48, "光照：");
            snprintf((char *)buf, sizeof(buf), "%d", set_light_threshold);
            OLED_ShowString(64, 48, (char *)buf, OLED_8X16);
        }
        else if (g_oled_page == 3)
        {
            OLED_ShowChinese(26, 0, "保存到卡片");
            OLED_ShowString(16, 16, "key1:ESC", OLED_8X16);
            OLED_ShowString(16, 32, "key2:store", OLED_8X16);
        }
        else
        {
            OLED_ShowString(16, 32, "ERROR!!!", OLED_8X16);
        }

        OLED_Update();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
