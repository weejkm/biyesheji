/**
 ******************************************************************************
 * @file    Key.c
 * @brief   按键驱动实现文件
 * @details 实现按键的初始化、消抖、长短按检测功能
 *          支持4个按键：PB0、PB4、PB5、PB9
 ******************************************************************************
 */

#include "stm32f10x.h"
#include "Delay.h"
#include "Key.h"

// ==================== 按键参数定义 ====================

/**
 * @brief 按键消抖时间（毫秒）
 * @note 检测到按键按下后，延时20ms再次检测，避免机械抖动
 */
#define KEY_DEBOUNCE_MS      20u

/**
 * @brief 按键长按判定时间（毫秒）
 * @note 按键按下时间达到1000ms时，判定为长按
 */
#define KEY_LONG_PRESS_MS  1000u

/**
 * @brief 按键扫描步进时间（毫秒）
 * @note 每10ms检测一次按键状态，用于计算按键按下时间
 */
#define KEY_SCAN_STEP_MS     10u

// ==================== 内部函数 ====================

/**
 * @brief 扫描单个按键的状态
 * @param port GPIO端口指针（如GPIOB）
 * @param pin GPIO引脚编号（如GPIO_Pin_4）
 * @return 按键状态：CLICK（短按）、LONGCLICK（长按）、NCLICK（无按键）
 * @details 实现步骤：
 *          1. 检测按键是否按下（低电平）
 *          2. 消抖延时后再检测
 *          3. 统计按键按下的持续时间
 *          4. 根据持续时间返回短按或长按状态
 * @note 该函数为阻塞式函数，会等待按键释放后才返回
 *       按键采用低电平有效连接方式（按下为低电平，松开为高电平）
 */
static KeyState Key_ScanPin(GPIO_TypeDef *port, uint16_t pin)
{
    uint16_t press_ms = 0;  // 按键按下持续时间计数器

    // 1. 首次检测：检查按键是否按下（低电平）
    if (GPIO_ReadInputDataBit(port, pin) != Bit_RESET) {
        return NCLICK;  // 按键未按下，返回无按键状态
    }

    // 2. 消抖处理：延时20ms后再次检测
    Delay_ms(KEY_DEBOUNCE_MS);
    if (GPIO_ReadInputDataBit(port, pin) != Bit_RESET) {
        return NCLICK;  // 消抖后按键松开，返回无按键状态
    }

    // 3. 循环检测：统计按键按下的持续时间
    while (GPIO_ReadInputDataBit(port, pin) == Bit_RESET) {
        Delay_ms(KEY_SCAN_STEP_MS);  // 延时10ms
        // 累计按键按下时间，防止计数器溢出
        if (press_ms < 0xFFFFu - KEY_SCAN_STEP_MS) {
            press_ms += KEY_SCAN_STEP_MS;
        }
    }

    // 4. 判断按键类型：根据持续时间返回短按或长按
    if (press_ms >= KEY_LONG_PRESS_MS) {
        return LONGCLICK;  // 按下时间>=1秒，判定为长按
    }
    return CLICK;  // 按下时间<1秒，判定为短按
}

// ==================== 按键初始化函数 ====================

/**
 * @brief 初始化按键驱动
 * @details 配置四个按键为上拉输入模式：
 *        KEY1: PB4
 *        KEY2: PB5
 *        KEY3: PB9
 *        KEY4: PB0
 */
void Key_Init(void)
{
    // 1. 使能GPIOB时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // 2. 配置GPIO引脚为上拉输入模式
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入模式
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_5 | GPIO_Pin_4 | GPIO_Pin_0;  // 四个按键引脚
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  // GPIO速度
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// ==================== 按键状态获取函数 ====================

/**
 * @brief 获取指定按键的状态
 * @param key 按键编号，取值范围为KEY1~KEY4
 * @return 按键状态：CLICK（短按）、LONGCLICK（长按）、NCLICK（无按键）
 * @note 根据按键编号映射到对应的GPIO引脚，然后调用按键扫描函数
 */
KeyState Key_GetState(KeyNum key)
{
    switch (key)
    {
        case KEY1:
            return Key_ScanPin(GPIOB, GPIO_Pin_4);  // 检测KEY1 (PB4)
        case KEY2:
            return Key_ScanPin(GPIOB, GPIO_Pin_5);  // 检测KEY2 (PB5)
        case KEY3:
            return Key_ScanPin(GPIOB, GPIO_Pin_9);  // 检测KEY3 (PB9)
        case KEY4:
            return Key_ScanPin(GPIOB, GPIO_Pin_0);  // 检测KEY4 (PB0)
        default:
            return NCLICK;  // 无效按键编号，返回无按键状态
    }
}

