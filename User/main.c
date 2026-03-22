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

//系统硬件初始化
void sys_init(void);
//开始页面（选择系统模式）
void start_page(void);

int main(void)
{
	sys_init();
	start_page();
	
    /* ======= tasks ======= */
	//创建OLED显示任务
	xTaskCreate(OLED_Task,      "OLED",   512, NULL, 1, NULL);
	//创建数据检测任务
    xTaskCreate(Detection_Task, "Detect", 256, NULL, 2, NULL);
	//创建按键等控制任务
    xTaskCreate(KEY_Task,       "KEY",    512, NULL, 3, NULL);
    
    if (g_online_mode) {//在线模式创建消息队列和MQTT传输任务
		g_mqtt_pub_q = xQueueCreate(8, sizeof(mqtt_pub_msg_t));
        xTaskCreate(EspMqtt_Task, "MQTT", 768, NULL, 2, NULL);
    }

	//开启任务调度
    vTaskStartScheduler();
    //系统开启调度后正常情况下不会执行到这里
    //while (1) { }
}

void sys_init(void){
	delay_init();
    USART1_Init(115200);
    OLED_Init();
	OLED_Clear();
	OLED_ShowString(16, 16, "please wait", OLED_8X16);
	OLED_Update();
    Key_Init();
    Init_ADC1();
    MyRTC_Init();
    ESP8266_Init(115200);
    LED_Init();
    Encoder_Init();
    RC522_SPI_Init();
    Servo_Init();
    SU_Init();
    Fan_Init();
	DHT11_Init();
}
void start_page(void){
	OLED_Clear();
	OLED_Update();
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
        {//按键1选择在线模式
            //g_online_mode = 1;
            break;
        }
        else if (Key_GetState(KEY2) == CLICK)
        {//按键二选择离线模式
            g_online_mode = 0;
            break;
        }

        Delay_ms(50);
        if (++OutTime >= 100) break; //定时5秒退出模式选择，选择默认模式->在线模式
    }
	OLED_Clear();
	OLED_ShowString(16, 16, "please wait", OLED_8X16);
	OLED_Update();
}
