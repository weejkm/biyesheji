#ifndef __SERVO__H__
#define __SERVO__H__


void Servo_Init(void);
void Servo_SetAngle(float Angle);
#define Servo_SetON  Servo_SetAngle(180);
#define Servo_SetOFF Servo_SetAngle(0);

#endif
