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

/* external globals from other modules */
extern int16_t Encoder_Count;

void KEY_Task(void *arg)
{
    int16_t tag;
    (void)arg;

    for (;;)
    {
        if (Key_GetState(KEY1) == CLICK)
        {
            g_oled_page = (g_oled_page + 1) % OLED_PAGE_COUNT;
            g_auto_mode = (g_oled_page == 1) ? 0 : 1;
        }
        else if (Key_GetState(KEY2) == CLICK)
        {
            if (g_oled_page == 1 || g_oled_page == 2)
            {
                g_oled_select = (g_oled_select + 1) % 3;
            }
            else if (g_oled_page == 3)
            {
                /* ֵ RFID */
                g_write_buf[0] = set_temp_threshold;
                g_write_buf[1] = set_hum_threshold;
                g_write_buf[2] = set_light_threshold;

                if (RC522_WriteBlock(1, 1, g_write_buf) == RC522_CARD_OK)
                    printf("RFID store OK\r\n");
                else
                    printf("RFID store FAIL\r\n");

                RC522_BeginNewSession();
            }
        }
        else if (Key_GetState(KEY3) == CLICK)
        {
            g_oled_page = 0;
        }

        /*  */
        if ((tag = Encoder_Get()) != 0)
        {
            Encoder_Count = 0;
            if (tag >= 1) tag = 1;
            else if (tag < 0) tag = -1;

            if (g_oled_page == 2)
            {
                /* ֵ */
                if (g_oled_select == 0)
                {
                    int v = (int)set_temp_threshold + tag;
                    if (v < 0) v = 0;
                    if (v > 31) v = 31;
                    set_temp_threshold = (uint8_t)v;
                }
                else if (g_oled_select == 1)
                {
                    int v = (int)set_hum_threshold + tag;
                    if (v < 0) v = 0;
                    if (v > 100) v = 100;
                    set_hum_threshold = (uint8_t)v;
                }
                else
                {
                    int v = (int)set_light_threshold + tag;
                    if (v < 0) v = 0;
                    if (v > 100) v = 100;
                    set_light_threshold = (uint8_t)v;
                }
            }
            else if (g_oled_page == 1)
            {
                /* ֶ豸 */
                if (g_oled_select == 0)
                {
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
                    /* LED  0..100 */
                    if (tag == 1)
                        dev_led_level = (dev_led_level + 1) % 101;
                    else
                        dev_led_level = (dev_led_level == 0) ? 100 : (dev_led_level - 1);

                    LED_Set_light(dev_led_level);
                }
            }
            else
            {
                /* ҳҲ LEDѡ */
                int v = (int)dev_led_level + tag;
                if (v < 0) v = 0;
                if (v > 100) v = 100;
                dev_led_level = (uint8_t)v;
                LED_Set_light(dev_led_level);
            }
        }

        /* RFID ȡֵ */
        if (g_oled_page == 2)
        {
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

        /* Զ */
        if (g_auto_mode)
        {
            if (dev_light_percent >= set_light_threshold + 5 || dev_light_percent <= set_light_threshold - 5)
            {
                if (dev_light_percent >= set_light_threshold + 5)
                {
                    Servo_SetOFF;
                    dev_curtain_state = CURTAIN_OFF;
                    if (dev_led_level >= 1) dev_led_level--;
                }
                else
                {
                    Servo_SetON;
                    dev_curtain_state = CURTAIN_ON;
                    if (dev_led_level < 100) dev_led_level++;
                }
                LED_Set_light(dev_led_level);
            }

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

        
        if (SU_Receive_flag)
        {
            uint8_t data = SU_data;
            SU_Receive_flag = 0;
            g_auto_mode = 0;
			printf("voice cmd = 0x%02X\r\n", data);

            switch (data)
            {
                case 0x01: dev_led_level = 50;  LED_Set_light(dev_led_level); break;
                case 0x02: dev_led_level += 10; if (dev_led_level > 100) dev_led_level = 100; LED_Set_light(dev_led_level); break;
                case 0x03: dev_led_level = (dev_led_level <= 10) ? 0 : (dev_led_level - 10); LED_Set_light(dev_led_level); break;
                case 0x04: dev_led_level = 0;   LED_Set_light(dev_led_level); break;
                case 0x05: dev_fan_state = FAN_ON;  Fan_set(FAN_ON);  break;
                case 0x06: dev_fan_state = FAN_OFF; Fan_set(FAN_OFF); break;
                case 0x07: dev_curtain_state = CURTAIN_ON;  Servo_SetON;  break;
                case 0x08: dev_curtain_state = CURTAIN_OFF; Servo_SetOFF;;break;
                case 0x09: g_auto_mode = 1;break;
                default: break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
