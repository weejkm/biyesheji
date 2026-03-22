#ifndef __KEY_H
#define __KEY_H

/**
 ******************************************************************************
 * @file    Key.h
 * @brief   按键驱动头文件
 * @details 本文件提供了按键初始化和状态检测函数
 *          支持短按、长按检测，实现按键消抖功能
 ******************************************************************************
 */

// ==================== 按键状态枚举 ====================

/**
 * @brief 按键状态枚举
 * @details 定义按键的三种状态：
 *          - CLICK: 短按（按下时间小于1秒）
 *          - LONGCLICK: 长按（按下时间大于等于1秒）
 *          - NCLICK: 无按键或未按下
 */
typedef enum{
    CLICK = 0,        // 短按状态
    LONGCLICK,        // 长按状态
    NCLICK            // 无按键状态
} KeyState;

// ==================== 按键编号枚举 ====================

/**
 * @brief 按键编号枚举
 * @details 定义四个按键的编号
 *          - KEY1: 连接在PB4
 *          - KEY2: 连接在PB5
 *          - KEY3: 连接在PB9
 *          - KEY4: 连接在PB0
 */
typedef enum{
    KEY1 = 0,         // 按键1 (PB4)
    KEY2,             // 按键2 (PB5)
    KEY3,             // 按键3 (PB9)
    KEY4,             // 按键4 (PB0)
} KeyNum;

// ==================== 函数声明 ====================

/**
 * @brief 初始化按键驱动
 * @note 配置PB0、PB4、PB5、PB9为上拉输入模式
 */
void Key_Init(void);

/**
 * @brief 获取按键状态
 * @param key 按键编号，取值范围为KEY1~KEY4
 * @return 按键状态：CLICK（短按）、LONGCLICK（长按）、NCLICK（无按键）
 * @note 该函数为阻塞式函数，会等待按键释放后才返回
 */
KeyState Key_GetState(KeyNum key);

#endif
