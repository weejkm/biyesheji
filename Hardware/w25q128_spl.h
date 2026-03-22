#ifndef __W25Q128_SPL_H__
#define __W25Q128_SPL_H__

#include "stm32f10x.h"

/* --- 命令 --- */
#define W25Q_CMD_READ_ID         0x9F
#define W25Q_CMD_READ_DATA       0x03
#define W25Q_CMD_FAST_READ       0x0B
#define W25Q_CMD_PAGE_PROGRAM    0x02
#define W25Q_CMD_SECTOR_ERASE    0x20   // 4KB
#define W25Q_CMD_BLOCK_ERASE_64K 0xD8
#define W25Q_CMD_CHIP_ERASE      0xC7
#define W25Q_CMD_WRITE_ENABLE    0x06
#define W25Q_CMD_READ_SR1        0x05
#define W25Q_CMD_WRITE_DISABLE   0x04

/* --- 常量 --- */
#define W25Q_PAGE_SIZE           256
#define W25Q_SECTOR_SIZE         (4*1024)

/* --- 引脚（PB12~PB15 -> SPI2） --- */
#define W25Q_CS_PORT             GPIOB
#define W25Q_CS_PIN              GPIO_Pin_12

#define W25Q_CS_LOW()            GPIO_ResetBits(W25Q_CS_PORT, W25Q_CS_PIN)
#define W25Q_CS_HIGH()           GPIO_SetBits(W25Q_CS_PORT, W25Q_CS_PIN)

/* API */
void    W25Q_SPI2_Init(void);                    // 使能时钟+GPIO+SPI2配置
uint8_t W25Q_ReadSR1(void);
void    W25Q_WriteEnable(void);
void    W25Q_WaitBusy(void);

uint32_t W25Q_ReadJEDECID(void);
void     W25Q_Read(uint32_t addr, uint8_t *buf, uint16_t len);
void     W25Q_PageProgram(uint32_t addr, const uint8_t *buf, uint16_t len);
void     W25Q_SectorErase(uint32_t addr);
void     W25Q_BlockErase64K(uint32_t addr);
void     W25Q_ChipErase(void);

#endif
