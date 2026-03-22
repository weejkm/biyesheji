#ifndef __KEY_H
#define __KEY_H


typedef enum{
    CLICK=0,
    LONGCLICK,
    NCLICK
}KeyState;

typedef enum{
    KEY1=0,
    KEY2,
    KEY3,
    KEY4,
}KeyNum;



void Key_Init(void);
KeyState Key_GetState(KeyNum key);

#endif
