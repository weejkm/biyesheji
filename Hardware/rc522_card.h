#ifndef __RC522_CARD_H__
#define __RC522_CARD_H__

#include <stdint.h>
#include "rc522.h"

/* 高层封装返回码（与底层一致的语义） */
#define RC522_CARD_OK   0U
#define RC522_CARD_ERR  1U



void RC522_BeginNewSession(void);

/* 设置读写使用的 KeyA（默认 FF FF FF FF FF FF） */
void RC522_SetKeyA(const uint8_t keyA[6]);

/* 寻卡：成功返回 RC522_CARD_OK，uid[0..3] 返回 4 字节 UID */
uint8_t RC522_Search(uint8_t uid[4]);

/* 读取：sector ∈ [0..15]，blockInSector ∈ [0..3]，recvData 必须可写入 16 字节 */
uint8_t RC522_ReadBlock(uint8_t sector, uint8_t blockInSector, uint8_t recvData[16]);

/* 写入：sector ∈ [0..15]，blockInSector ∈ [0..3]，writeData 必须为 16 字节 */
uint8_t RC522_WriteBlock(uint8_t sector, uint8_t blockInSector, const uint8_t writeData[16]);


#endif /* __RC522_CARD_H__ */
