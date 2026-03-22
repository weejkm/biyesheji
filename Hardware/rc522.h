#ifndef __RC522_H
#define __RC522_H

#include "stm32f10x.h"

//--------------------------------------------------
// RC522 寄存器地址定义（依据 NXP MFRC522 数据手册）
//--------------------------------------------------
#define MFRC522_REG_COMMAND              0x01
#define MFRC522_REG_COMM_IE_N            0x02
#define MFRC522_REG_DIV_IRQ              0x05
#define MFRC522_REG_COMM_IRQ             0x04
#define MFRC522_REG_ERROR                0x06
#define MFRC522_REG_STATUS1              0x07
#define MFRC522_REG_STATUS2              0x08
#define MFRC522_REG_FIFO_DATA            0x09
#define MFRC522_REG_FIFO_LEVEL           0x0A
#define MFRC522_REG_CONTROL              0x0C
#define MFRC522_REG_BIT_FRAMING          0x0D
#define MFRC522_REG_COLL                 0x0E

// 命令集寄存器组
#define MFRC522_REG_MODE                 0x11
#define MFRC522_REG_TX_MODE              0x12
#define MFRC522_REG_RX_MODE              0x13
#define MFRC522_REG_TX_CONTROL           0x14
#define MFRC522_REG_TX_AUTO              0x15
#define MFRC522_REG_T_MODE               0x2A
#define MFRC522_REG_T_PRESCALER          0x2B
#define MFRC522_REG_T_RELOAD_H           0x2C
#define MFRC522_REG_T_RELOAD_L           0x2D



#define MFRC522_REG_CRC_RESULT_H         0x21
#define MFRC522_REG_CRC_RESULT_L         0x22
#define MFRC522_REG_CRC_RESULT_M         MFRC522_REG_CRC_RESULT_H  // 为兼容 .c 文件中的命名

#define MFRC522_REG_VERSION              0x37   // 芯片版本号寄存器

//--------------------------------------------------
// MFRC522 命令字
//--------------------------------------------------
#define PCD_IDLE             0x00
#define PCD_AUTHENT          0x0E
#define PCD_RECEIVE          0x08
#define PCD_TRANSMIT         0x04
#define PCD_TRANSCEIVE       0x0C
#define PCD_RESETPHASE       0x0F
#define PCD_CALCCRC          0x03

//--------------------------------------------------
// Mifare 卡命令字
//--------------------------------------------------
#define PICC_REQIDL          0x26
#define PICC_REQALL          0x52
#define PICC_ANTICOLL        0x93
#define PICC_SElECTTAG       0x93
#define PICC_AUTHENT1A       0x60
#define PICC_AUTHENT1B       0x61
#define PICC_READ            0x30
#define PICC_WRITE           0xA0
#define PICC_DECREMENT       0xC0
#define PICC_INCREMENT       0xC1
#define PICC_RESTORE         0xC2
#define PICC_TRANSFER        0xB0
#define PICC_HALT            0x50

//--------------------------------------------------
// 状态返回值
//--------------------------------------------------
#define MI_OK                0
#define MI_NOTAGERR          1
#define MI_ERR               2

//--------------------------------------------------
// 函数声明
//--------------------------------------------------
void RC522_SPI_Init(void);
void RC522_WriteReg(uint8_t addr, uint8_t val);
uint8_t RC522_ReadReg(uint8_t addr);
void RC522_SetBitMask(uint8_t reg, uint8_t mask);
void RC522_ClearBitMask(uint8_t reg, uint8_t mask);

void RC522_Reset(void);
void RC522_AntennaOn(void);
void RC522_AntennaOff(void);

uint8_t RC522_ComMF522(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen);
uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType);
uint8_t RC522_Anticoll(uint8_t *serNum);
uint8_t RC522_SelectTag(uint8_t *serNum);
uint8_t RC522_Check(uint8_t *id);
void RC522_CalculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData);
void RC522_Halt(void);

#endif
