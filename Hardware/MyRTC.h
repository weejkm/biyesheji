#ifndef __MYRTC_H
#define __MYRTC_H


void MyRTC_Init(void);
void MyRTC_SetTime(void);
void MyRTC_ReadTime(void);
int parse_time_and_set_rtc(const char *buf);

#endif
