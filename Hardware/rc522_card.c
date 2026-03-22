#include "rc522.h"
#include <string.h>
#include "delay.h"
#include "stdio.h"

/* 默认 KeyA（多数 Mifare S50 出厂为全 0xFF） */
static uint8_t s_keyA[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/* 可选：外部修改 KeyA */
void RC522_SetKeyA(const uint8_t keyA[6])
{
    if (keyA) memcpy(s_keyA, keyA, 6);
}

static uint8_t s_keyB[6] = {0xFF,0xFF,0xFF,0xFF,0xFF};
void RC522_SetKeyB(const uint8_t keyB[6]) 
{ 
	if (keyB) memcpy(s_keyB, keyB, 6); 
}


void RC522_BeginNewSession(void)
{
    // 1) 停止命令、清 FIFO、退出加密、清位对齐
    RC522_WriteReg(MFRC522_REG_COMMAND, PCD_IDLE);
    RC522_WriteReg(MFRC522_REG_FIFO_LEVEL, 0x80);     // 清 FIFO
    RC522_ClearBitMask(MFRC522_REG_STATUS2, 0x08);    // Crypto1Off
    RC522_WriteReg(MFRC522_REG_BIT_FRAMING, 0x00);    // 位对齐清零

    // 2) 恢复关键通信参数（与常见参考初始化一致）
    RC522_WriteReg(MFRC522_REG_MODE,      0x3D);      // CRC 初值 0x6363
    RC522_WriteReg(MFRC522_REG_TX_MODE,   0x00);
    RC522_WriteReg(MFRC522_REG_RX_MODE,   0x00);

    // 说明：你的 0x15 宏名叫 TX_AUTO，但该地址其实是 TxASK 寄存器（Force100%ASK 在 bit6）
    RC522_WriteReg(MFRC522_REG_TX_AUTO,   0x40);      // Force 100% ASK

    RC522_WriteReg(MFRC522_REG_T_MODE,        0x8D);  // 定时器自动
    RC522_WriteReg(MFRC522_REG_T_PRESCALER,   0x3E);
    RC522_WriteReg(MFRC522_REG_T_RELOAD_H,    0x00);
    RC522_WriteReg(MFRC522_REG_T_RELOAD_L,    30);

    // 3) RF 场重启（模拟离场再入场）
    RC522_AntennaOff();  
    Delay_ms(20);
    RC522_AntennaOn();   
    Delay_ms(50);

}



/* ----------------- 本文件内部使用的局部帮助函数 ----------------- */

/* 认证：直接用 RC522_ComMF522 完成，不依赖 rc522.c 里的 RC522_Auth 封装 */
static uint8_t rc522_auth_local(uint8_t authMode, uint8_t blockAddr,
                                uint8_t *key, uint8_t *serNum4)
{
    uint8_t status;
    uint16_t recvBits;
    uint8_t i;
    uint8_t frame[12];

    frame[0] = authMode;        // PICC_AUTHENT1A(0x60) / PICC_AUTHENT1B(0x61)
    frame[1] = blockAddr;       // 绝对块号
    for (i = 0; i < 6; i++) frame[i + 2] = key[i];
    for (i = 0; i < 4; i++) frame[i + 8] = serNum4[i];

    status = RC522_ComMF522(PCD_AUTHENT, frame, 12, frame, &recvBits);

    // 验证是否进入加密状态（Status2 寄存器 MFCrypto1On 位 = 1）
    if ((status != MI_OK) || (!(RC522_ReadReg(MFRC522_REG_STATUS2) & 0x08)))
        status = MI_ERR;

    return status;
}

/* 读块：直接组帧+调用 RC522_ComMF522，不依赖 rc522.c 的 RC522_Read */
static uint8_t rc522_read_local(uint8_t blockAddr, uint8_t *recv16)
{
    uint8_t status;
    uint16_t backBits;
    uint8_t cmd[4];

    cmd[0] = PICC_READ;
    cmd[1] = blockAddr;
    RC522_CalculateCRC(cmd, 2, &cmd[2]);

    status = RC522_ComMF522(PCD_TRANSCEIVE, cmd, 4, recv16, &backBits);
    // 期望 16 数据 + 2 CRC = 18 字节 = 144 bit = 0x90
    if ((status != MI_OK) || (backBits != 0x90))
        status = MI_ERR;

    return status;
}

/* 写块：直接组帧+两步握手，不依赖 rc522.c 的 RC522_Write */
static uint8_t rc522_write_local(uint8_t blockAddr, const uint8_t *data16)
{
    uint8_t status;
    uint16_t backBits;
    uint8_t ack[2];
    uint8_t cmd[4];

    // ---- Step 1: 写命令 ----
    cmd[0] = PICC_WRITE;   // 0xA0
    cmd[1] = blockAddr;
    RC522_CalculateCRC(cmd, 2, &cmd[2]);

    Delay_ms(2);  // ★ 新增延时 2ms，给卡反应时间

    status = RC522_ComMF522(PCD_TRANSCEIVE, cmd, 4, ack, &backBits);
    if ((status != MI_OK) || (backBits != 4) || ((ack[0] & 0x0F) != 0x0A))
        return MI_ERR;

    // ---- Step 2: 发送16字节数据 + CRC ----
    uint8_t frame[18];
    memcpy(frame, data16, 16);
    RC522_CalculateCRC(frame, 16, &frame[16]);

    Delay_ms(2);  // ★ 再延时 2ms

    status = RC522_ComMF522(PCD_TRANSCEIVE, frame, 18, ack, &backBits);
    if ((status != MI_OK) || (backBits != 4) || ((ack[0] & 0x0F) != 0x0A))
        return MI_ERR;

    return MI_OK;
}

/* ----------------- 对外的高层接口 ----------------- */

/* 寻卡：成功返回 MI_OK（你也可以在外层把它映射为 RC522_CARD_OK） */
uint8_t RC522_Search(uint8_t uid[4])
{
    uint8_t status;
    uint8_t tmp[5];

    // 1) 寻卡
    status = RC522_Request(PICC_REQIDL, tmp);
    if (status != MI_OK) return MI_ERR;

    // 2) 防冲突（tmp[0..4]，最后一字节为校验）
    status = RC522_Anticoll(tmp);
    if (status != MI_OK) return MI_ERR;

    if (uid) {
        uid[0] = tmp[0];
        uid[1] = tmp[1];
        uid[2] = tmp[2];
        uid[3] = tmp[3];
    }
    return MI_OK;
}

/* 读取：sector ∈ [0..15]，blockInSector ∈ [0..3]，recvData 为 16 字节缓冲 */
uint8_t RC522_ReadBlock(uint8_t sector, uint8_t blockInSector, uint8_t recvData[16])
{
	RC522_Halt();
    if (recvData == NULL || blockInSector > 3U) return MI_ERR;

    uint8_t status;
    uint8_t id[5];
    uint8_t blockAddr = (uint8_t)(sector * 4U + blockInSector);

    // 1) 寻卡
    status = RC522_Request(PICC_REQIDL, id);
    if (status != MI_OK) {
		printf("寻卡失败\r\n");
		return MI_ERR;
	}

    // 2) 防冲突
    status = RC522_Anticoll(id);
    if (status != MI_OK) {
		printf("防冲突失败\r\n");
		return MI_ERR;
	}

    // 3) 选卡
    status = RC522_SelectTag(id);
    if (status != MI_OK) {
		printf("选卡失败\r\n");
		return MI_ERR;
	}

    // 4) 认证（KeyA）
    status = rc522_auth_local(PICC_AUTHENT1A, blockAddr, s_keyA, id);
    if (status != MI_OK) { RC522_Halt(); return MI_ERR; }

    // 5) 读取
    status = rc522_read_local(blockAddr, recvData);

    // 6) 停止通信
    RC522_Halt();

    return status; // MI_OK / MI_ERR
}

/* 写入：sector ∈ [0..15]，blockInSector ∈ [0..3]，writeData 为 16 字节 */
uint8_t RC522_WriteBlock(uint8_t sector, uint8_t blockInSector, const uint8_t writeData[16])
{
	
	RC522_Halt();
    if (writeData == NULL || blockInSector > 3U) return MI_ERR;
	
    uint8_t status;
    uint8_t id[5];
    uint8_t blockAddr = (uint8_t)(sector * 4U + blockInSector);

    // 1) 寻卡
    status = RC522_Request(PICC_REQIDL, id);
    if (status != MI_OK) {
		printf("寻卡失败\r\n");
		return MI_ERR;
	}

    // 2) 防冲突
    status = RC522_Anticoll(id);
    if (status != MI_OK) {
		printf("放冲突失败\r\n");
		return MI_ERR;
	}

    // 3) 选卡
    status = RC522_SelectTag(id);
    if (status != MI_OK) {
		printf("选卡失败\r\n");
		return MI_ERR;
	}

    // 4) 先用 KeyA 认证
    status = rc522_auth_local(PICC_AUTHENT1A, blockAddr, s_keyA, id);
    if (status == MI_OK) {
        status = rc522_write_local(blockAddr, writeData);
        if (status == MI_OK) { RC522_Halt(); return MI_OK; }
    }
	printf("KeyA 认证失败\r\n");

    // 5) A 写失败，尝试 KeyB
    status = rc522_auth_local(PICC_AUTHENT1B, blockAddr, s_keyB, id);
    if (status == MI_OK) {
        status = rc522_write_local(blockAddr, writeData);
        RC522_Halt();
        return status;
    }
	printf("KeyB 认证失败\r\n");

    RC522_Halt();
    return MI_ERR;
}

