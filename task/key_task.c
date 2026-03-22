#include "stm32f10x.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#include "Key.h"
#include "Encoder.h"
#include "Servo.h"
#include "Fan.h"
#include "LED.h"
#include "SU.h"
#include "rc522.h"
#include "rc522_card.h"

#include "app_state.h"

/* 其他模块中定义的全局变量 */
extern int16_t Encoder_Count;

/* =========================================================================
 * 按键与人机交互任务
 *
 * 功能说明：
 * 1. 处理按键 KEY1 / KEY2 / KEY3 的操作；
 * 2. 处理旋转编码器输入；
 * 3. 在不同 OLED 页面下，实现不同的控制逻辑；
 * 4. 支持 RFID 读写阈值参数；
 * 5. 自动模式下，根据光照和温度自动控制窗帘、LED、风扇；
 * 6. 支持语音模块 SU 的指令控制。
 *
 * 页面逻辑大致如下：
 * - 页面 0：默认页面，可调节 LED
 * - 页面 1：设备手动控制页面（窗帘 / 风扇 / LED）
 * - 页面 2：阈值设置页面（温度 / 湿度 / 光照阈值）
 * - 页面 3：RFID 写卡页面
 * ========================================================================= */
void KEY_Task(void *arg)
{
    int16_t tag;
    (void)arg;

    for (;;)
    {
        /* ===================== 按键处理 ===================== */
        if (Key_GetState(KEY1) == CLICK)
        {
            /* 
             * KEY1 单击：
             * OLED 页面循环切换
             * 切换到页面1时进入手动模式，其余页面默认自动模式
             */
            g_oled_page = (g_oled_page + 1) % OLED_PAGE_COUNT;
            g_auto_mode = (g_oled_page == 1) ? 0 : 1;
        }
        else if (Key_GetState(KEY2) == CLICK)
        {
            if (g_oled_page == 1 || g_oled_page == 2)
            {
                /*
                 * KEY2 单击：
                 * 在页面1（设备控制）或页面2（阈值设置）中，
                 * 用于切换当前选中的项目：
                 * 0 / 1 / 2 分别对应三个可操作对象
                 */
                g_oled_select = (g_oled_select + 1) % 3;
            }
            else if (g_oled_page == 3)
            {
                /*
                 * 页面3下，KEY2 单击执行 RFID 写卡：
                 * 将当前设置的温度、湿度、光照阈值写入 RFID 卡。
                 */
                g_write_buf[0] = set_temp_threshold;
                g_write_buf[1] = set_hum_threshold;
                g_write_buf[2] = set_light_threshold;

                if (RC522_WriteBlock(1, 1, g_write_buf) == RC522_CARD_OK)
                    printf("RFID store OK\r\n");
                else
                    printf("RFID store FAIL\r\n");

                /* 写卡结束后开启新的会话，避免重复操作 */
                RC522_BeginNewSession();
            }
        }
        else {
            KeyState key3_state = Key_GetState(KEY3);
            if (key3_state == CLICK)
            {
                /*
                 * KEY3 单击：
                 * 返回首页
                 */
                g_oled_page = 0;
            }
            else if (key3_state == LONGCLICK)
            {
                /*
                 * KEY3 长按：
                 * 在线模式 / 离线模式切换
                 * 同时回到首页
                 */
                g_online_mode = !g_online_mode;
                g_oled_page = 0;
            }
        }

        /* ===================== 编码器处理 ===================== */
        if ((tag = Encoder_Get()) != 0)
        {
            /*
             * 读取旋转编码器增量：
             * tag > 0 表示顺时针
             * tag < 0 表示逆时针
             * 这里将其归一化为 +1 或 -1
             */
            Encoder_Count = 0;
            if (tag >= 1) tag = 1;
            else if (tag < 0) tag = -1;

            if (g_oled_page == 2)
            {
                /*
                 * 页面2：阈值设置页面
                 * 根据当前选中项调整阈值
                 */
                if (g_oled_select == 0)
                {
                    /* 调节温度阈值，范围 0~31 */
                    int v = (int)set_temp_threshold + tag;
                    if (v < 0) v = 0;
                    if (v > 31) v = 31;
                    set_temp_threshold = (uint8_t)v;
                }
                else if (g_oled_select == 1)
                {
                    /* 调节湿度阈值，范围 0~100 */
                    int v = (int)set_hum_threshold + tag;
                    if (v < 0) v = 0;
                    if (v > 100) v = 100;
                    set_hum_threshold = (uint8_t)v;
                }
                else
                {
                    /* 调节光照阈值，范围 0~100 */
                    int v = (int)set_light_threshold + tag;
                    if (v < 0) v = 0;
                    if (v > 100) v = 100;
                    set_light_threshold = (uint8_t)v;
                }
            }
            else if (g_oled_page == 1)
            {
                /*
                 * 页面1：手动控制页面
                 * 根据当前选择项控制对应设备
                 */
                if (g_oled_select == 0)
                {
                    /* 控制窗帘：顺时针开，逆时针关 */
                    if (tag == 1 && dev_curtain_state != CURTAIN_ON)
                    {
                        dev_curtain_state = CURTAIN_ON;
                        Servo_SetON;
                    }
                    else if (tag == -1 && dev_curtain_state != CURTAIN_OFF)
                    {
                        dev_curtain_state = CURTAIN_OFF;
                        Servo_SetOFF;
                    }
                }
                else if (g_oled_select == 1)
                {
                    /* 控制风扇：顺时针开，逆时针关 */
                    if (tag == 1)
                    {
                        if (dev_fan_state != FAN_ON)
                        {
                            dev_fan_state = FAN_ON;
                            Fan_set(FAN_ON);
                        }
                    }
                    else if (tag == -1)
                    {
                        if (dev_fan_state != FAN_OFF)
                        {
                            dev_fan_state = FAN_OFF;
                            Fan_set(FAN_OFF);
                        }
                    }
                }
                else
                {
                    /* 控制 LED 亮度，范围 0~100，循环调节 */
                    if (tag == 1)
                        dev_led_level = (dev_led_level + 1) % 101;
                    else
                        dev_led_level = (dev_led_level == 0) ? 100 : (dev_led_level - 1);

                    LED_Set_light(dev_led_level);
                }
            }
            else
            {
                /*
                 * 其他页面（例如首页）：
                 * 编码器默认用于直接调节 LED 亮度
                 */
                int v = (int)dev_led_level + tag;
                if (v < 0) v = 0;
                if (v > 100) v = 100;
                dev_led_level = (uint8_t)v;
                LED_Set_light(dev_led_level);
            }
        }

        /* ===================== RFID 读卡处理 ===================== */
        if (g_oled_page == 2)
        {
            /*
             * 页面2下允许刷 RFID 卡读取阈值：
             * 若检测到卡片，则从指定块中读取温度/湿度/光照阈值，
             * 并更新到当前系统参数中。
             */
            if (RC522_Search(g_card_id) == RC522_CARD_OK)
            {
                if (RC522_ReadBlock(1, 1, g_read_buf) == RC522_CARD_OK)
                {
                    RC522_BeginNewSession();
                    set_temp_threshold  = g_read_buf[0];
                    set_hum_threshold   = g_read_buf[1];
                    set_light_threshold = g_read_buf[2];
                }
                RC522_BeginNewSession();
            }
        }

        /* ===================== 自动模式控制 ===================== */
        if (g_auto_mode)
        {
            /*
             * 根据环境光强自动控制窗帘和 LED：
             * 采用 ±5 的滞回区间，避免在临界值附近频繁抖动。
             */
            if (dev_light_percent >= set_light_threshold + 5 || dev_light_percent <= set_light_threshold - 5)
            {
                if (dev_light_percent >= set_light_threshold + 5)
                {
                    /*
                     * 当前光照偏强：
                     * - 关闭窗帘
                     * - LED 亮度适当减小
                     */
                    Servo_SetOFF;
                    dev_curtain_state = CURTAIN_OFF;
                    if (dev_led_level >= 1) dev_led_level--;
                }
                else
                {
                    /*
                     * 当前光照偏弱：
                     * - 打开窗帘
                     * - LED 亮度适当增加
                     */
                    Servo_SetON;
                    dev_curtain_state = CURTAIN_ON;
                    if (dev_led_level < 100) dev_led_level++;
                }
                LED_Set_light(dev_led_level);
            }

            /*
             * 根据温度阈值自动控制风扇：
             * 温度高于阈值则开风扇，否则关风扇
             */
            if (dev_dht11.temperature_int > set_temp_threshold)
            {
                dev_fan_state = FAN_ON;
                Fan_set(FAN_ON);
            }
            else
            {
                dev_fan_state = FAN_OFF;
                Fan_set(FAN_OFF);
            }
        }

        /* ===================== 语音指令处理 ===================== */
        if (SU_Receive_flag)
        {
            uint8_t data = SU_data;
            SU_Receive_flag = 0;

            /*
             * 收到语音指令后，默认退出自动模式，进入手动控制模式
             */
            g_auto_mode = 0;

            printf("voice cmd = 0x%02X\r\n", data);

            switch (data)
            {
                case 0x01:
                    /* 设置 LED 亮度为 50% */
                    dev_led_level = 50;
                    LED_Set_light(dev_led_level);
                    break;

                case 0x02:
                    /* LED 亮度增加 10% */
                    dev_led_level += 10;
                    if (dev_led_level > 100) dev_led_level = 100;
                    LED_Set_light(dev_led_level);
                    break;

                case 0x03:
                    /* LED 亮度减少 10% */
                    dev_led_level = (dev_led_level <= 10) ? 0 : (dev_led_level - 10);
                    LED_Set_light(dev_led_level);
                    break;

                case 0x04:
                    /* 关闭 LED */
                    dev_led_level = 0;
                    LED_Set_light(dev_led_level);
                    break;

                case 0x05:
                    /* 打开风扇 */
                    dev_fan_state = FAN_ON;
                    Fan_set(FAN_ON);
                    break;

                case 0x06:
                    /* 关闭风扇 */
                    dev_fan_state = FAN_OFF;
                    Fan_set(FAN_OFF);
                    break;

                case 0x07:
                    /* 打开窗帘 */
                    dev_curtain_state = CURTAIN_ON;
                    Servo_SetON;
                    break;

                case 0x08:
                    /* 关闭窗帘 */
                    dev_curtain_state = CURTAIN_OFF;
                    Servo_SetOFF;
                    break;

                case 0x09:
                    /* 切换到自动模式 */
                    g_auto_mode = 1;
                    break;

                default:
                    /* 未识别的语音指令，不做处理 */
                    break;
            }
        }

        /* 任务周期 100ms */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
