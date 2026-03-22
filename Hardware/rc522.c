#include "rc522.h"
#include "Delay.h"

//--------------------------------------------------
// 引脚定义：CS -> PA4, RST -> PB3
//--------------------------------------------------
#define RC522_CS_LOW()   GPIO_ResetBits(GPIOA, GPIO_Pin_4)
#define RC522_CS_HIGH()  GPIO_SetBits(GPIOA, GPIO_Pin_4)
#define RC522_RST_LOW()  GPIO_ResetBits(GPIOB, GPIO_Pin_3)
#define RC522_RST_HIGH() GPIO_SetBits(GPIOB, GPIO_Pin_3)

//--------------------------------------------------
// 常量定义
//--------------------------------------------------
#define RC522_DEFAULT_TIMEOUT 2000U
#define RC522_FIFO_SIZE       64U

//--------------------------------------------------
// 内部函数声明
//--------------------------------------------------
static uint8_t SPI1_ReadWriteByte(uint8_t TxData);

//--------------------------------------------------
// 初始化 SPI1 与 RC522 模块
//--------------------------------------------------
void RC522_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef  SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_GPIOB| RCC_APB2Periph_SPI1, ENABLE);

    // PA5: SCK, PA7: MOSI -> 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA6: MISO -> 输入浮空
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // PA4: CS
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	//PB3: RST -> 普通推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    RC522_CS_HIGH();
    RC522_RST_HIGH();

    // SPI 配置
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    SPI_Cmd(SPI1, ENABLE);

    // 初始化 RC522
    RC522_Reset();
    Delay_ms(50);

    // 寄存器配置
    RC522_WriteReg(MFRC522_REG_T_MODE, 0x8D);
    RC522_WriteReg(MFRC522_REG_T_PRESCALER, 0x3E);
    RC522_WriteReg(MFRC522_REG_T_RELOAD_L, 30);
    RC522_WriteReg(MFRC522_REG_T_RELOAD_H, 0);
    RC522_WriteReg(MFRC522_REG_TX_AUTO, 0x40);
    RC522_WriteReg(MFRC522_REG_MODE, 0x3D);

    RC522_AntennaOn();
    Delay_ms(10);
}

//--------------------------------------------------
// SPI 读写一个字节
//--------------------------------------------------
static uint8_t SPI1_ReadWriteByte(uint8_t TxData)
{
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, TxData);
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t)SPI_I2S_ReceiveData(SPI1);
}

//--------------------------------------------------
// 寄存器操作
//--------------------------------------------------
void RC522_WriteReg(uint8_t addr, uint8_t val)
{
    RC522_CS_LOW();
    SPI1_ReadWriteByte((addr << 1) & 0x7E);
    SPI1_ReadWriteByte(val);
    RC522_CS_HIGH();
}

uint8_t RC522_ReadReg(uint8_t addr)
{
    uint8_t val;
    RC522_CS_LOW();
    SPI1_ReadWriteByte(((addr << 1) & 0x7E) | 0x80);
    val = SPI1_ReadWriteByte(0x00);
    RC522_CS_HIGH();
    return val;
}

void RC522_SetBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = RC522_ReadReg(reg);
    RC522_WriteReg(reg, tmp | mask);
}

void RC522_ClearBitMask(uint8_t reg, uint8_t mask)
{
    uint8_t tmp = RC522_ReadReg(reg);
    RC522_WriteReg(reg, tmp & (~mask));
}

//--------------------------------------------------
// 设备控制
//--------------------------------------------------
void RC522_Reset(void)
{
    RC522_RST_LOW();
    Delay_ms(10);
    RC522_RST_HIGH();
    Delay_ms(10);
    RC522_WriteReg(MFRC522_REG_COMMAND, PCD_RESETPHASE);
}

void RC522_AntennaOn(void)
{
    uint8_t temp = RC522_ReadReg(MFRC522_REG_TX_CONTROL);
    if (!(temp & 0x03))
        RC522_SetBitMask(MFRC522_REG_TX_CONTROL, 0x03);
}

void RC522_AntennaOff(void)
{
    RC522_ClearBitMask(MFRC522_REG_TX_CONTROL, 0x03);
}

//--------------------------------------------------
// 通信核心函数
//--------------------------------------------------
/**
 * 函数名称：RC522_ComMF522
 * 功    能：向 MFRC522 芯片发送命令 + 数据，并接收返回数据（读卡核心）
 * 参    数：
 *    command  —— 要执行的 RC522 命令（如认证命令、发送/接收命令）
 *    sendData —— 发送给 RC522 的数据缓冲区
 *    sendLen  —— 发送数据长度（字节）
 *    backData —— RC522 回传的数据缓冲区（输出）
 *    backLen  —— RC522 回传的数据长度（以 bit 为单位）（输出）
 * 返 回 值：MI_OK（成功）、MI_ERR（失败）、MI_NOTAGERR（无卡或未响应）
 */
uint8_t RC522_ComMF522(uint8_t command, uint8_t *sendData, uint8_t sendLen,
                       uint8_t *backData, uint16_t *backLen)
{
    uint8_t status = MI_ERR;   // 默认状态：错误
    uint8_t irqEn = 0x00;      // 中断允许寄存器内容
    uint8_t waitIRq = 0x00;    // 等待的中断标志
    uint8_t lastBits;          // 最后不足 8 bit 的位数
    uint8_t n;
    uint16_t i;

    /* 选择命令后，要设置 MFRC522 的中断掩码与等待标志 */
    switch (command)
    {
        case PCD_AUTHENT:      // 验证密钥命令（认证）
            irqEn = 0x12;      // 允许的中断：ErrIEn + TimerIEn
            waitIRq = 0x10;    // 等待的中断：IdleIRq
            break;

        case PCD_TRANSCEIVE:   // 发送并接收（用于读卡）
            irqEn = 0x77;      // 开启所有相关中断
            waitIRq = 0x30;    // 等待 RxIRq 或 IdleIRq
            break;
    }

    /* 启用中断（CommIEn）并设置全局中断位（7 个中断） */
    RC522_WriteReg(MFRC522_REG_COMM_IE_N, irqEn | 0x80);

    /* 清除所有中断标志 */
    RC522_ClearBitMask(MFRC522_REG_COMM_IRQ, 0x80);

    /* FIFO 清空 */
    RC522_SetBitMask(MFRC522_REG_FIFO_LEVEL, 0x80);

    /* 进入 Idle 模式，准备发送命令 */
    RC522_WriteReg(MFRC522_REG_COMMAND, PCD_IDLE);

    /* 将 sendData 写入 FIFO */
    for (i = 0; i < sendLen; i++)
        RC522_WriteReg(MFRC522_REG_FIFO_DATA, sendData[i]);

    /* 执行命令（PCD_AUTHENT 或 PCD_TRANSCEIVE） */
    RC522_WriteReg(MFRC522_REG_COMMAND, command);

    if (command == PCD_TRANSCEIVE)
        RC522_SetBitMask(MFRC522_REG_BIT_FRAMING, 0x80);  // 启动发送

    /* 自旋等待命令完成（带超时机制） */
    i = RC522_DEFAULT_TIMEOUT;
    do
    {
        n = RC522_ReadReg(MFRC522_REG_COMM_IRQ);    // 读取中断标志寄存器
        i--;
    } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));
    // 条件说明：
    // i != 0          → 没超时
    // !(n & 0x01)     → TimerIRq 未触发（未超时）
    // !(n & waitIRq)  → 要等待的中断未触发（未完成）

    /* 清除自动发送位 */
    RC522_ClearBitMask(MFRC522_REG_BIT_FRAMING, 0x80);

    /* 如果没有超时 */
    if (i != 0)
    {
        /* 检查是否发生错误 */
        if (!(RC522_ReadReg(MFRC522_REG_ERROR) & 0x1B))
        {
            status = MI_OK;

            /* 如果是 Idle 状态，则表示没有卡响应 */
            if (n & irqEn & 0x01)
                status = MI_NOTAGERR;

            /* 如果是发送+接收指令，需要读取接收到的数据 */
            if (command == PCD_TRANSCEIVE)
            {
                n = RC522_ReadReg(MFRC522_REG_FIFO_LEVEL);   // FIFO 中的字节数量
                lastBits = RC522_ReadReg(MFRC522_REG_CONTROL) & 0x07; // 最后一个字节的有效位数

                if (lastBits)
                    *backLen = (n - 1) * 8 + lastBits;   // 总 bit 数
                else
                    *backLen = n * 8;

                if (n == 0)
                    n = 1;

                if (n > 16)
                    n = 16; // FIFO 最多存 16 字节

                /* 读取 FIFO 数据输出 */
                for (i = 0; i < n; i++)
                    backData[i] = RC522_ReadReg(MFRC522_REG_FIFO_DATA);
            }
        }
        else
            status = MI_ERR; // 发生硬件错误
    }

    return status;
}


//--------------------------------------------------
// 卡片操作函数
//--------------------------------------------------
uint8_t RC522_Request(uint8_t reqMode, uint8_t *TagType)
{
    uint8_t status;
    uint16_t backBits;

    RC522_WriteReg(MFRC522_REG_BIT_FRAMING, 0x07);
    TagType[0] = reqMode;
    status = RC522_ComMF522(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

    if ((status != MI_OK) || (backBits != 0x10))
        status = MI_ERR;
    return status;
}

uint8_t RC522_Anticoll(uint8_t *serNum)
{
    uint8_t status;
    uint8_t serNumCheck = 0;
    uint16_t unLen;

    RC522_WriteReg(MFRC522_REG_BIT_FRAMING, 0x00);
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = RC522_ComMF522(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK)
    {
        for (uint8_t i = 0; i < 4; i++)
            serNumCheck ^= serNum[i];
        if (serNumCheck != serNum[4])
            status = MI_ERR;
    }
    return status;
}

uint8_t RC522_SelectTag(uint8_t *serNum)
{
    uint8_t i;
    uint8_t status;
    uint8_t buffer[9];
    uint16_t recvBits;

    buffer[0] = PICC_SElECTTAG;
    buffer[1] = 0x70;
    for (i = 0; i < 5; i++)
        buffer[i + 2] = serNum[i];

    RC522_CalculateCRC(buffer, 7, &buffer[7]);
    status = RC522_ComMF522(PCD_TRANSCEIVE, buffer, 9, buffer, &recvBits);

    if ((status == MI_OK) && (recvBits == 0x18))
        return MI_OK;
    else
        return MI_ERR;
}

uint8_t RC522_Check(uint8_t *id)
{
    uint8_t status;
    uint8_t str[2];

    status = RC522_Request(PICC_REQALL, str);
    if (status == MI_OK)
    {
        status = RC522_Anticoll(id);
        if (status == MI_OK)
        {
            status = RC522_SelectTag(id);
            RC522_Halt();
        }
    }
    return status;
}

//--------------------------------------------------
// CRC 与 HALT
//--------------------------------------------------
void RC522_CalculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData)
{
    uint16_t i;    // 改为 uint16_t，避免 0x7FF 截断
    uint8_t n;

    RC522_WriteReg(MFRC522_REG_COMMAND, PCD_IDLE);
    RC522_WriteReg(MFRC522_REG_DIV_IRQ, 0x04);
    RC522_WriteReg(MFRC522_REG_FIFO_LEVEL, 0x80);

    for (uint8_t j = 0; j < len; j++)
        RC522_WriteReg(MFRC522_REG_FIFO_DATA, pIndata[j]);

    RC522_WriteReg(MFRC522_REG_COMMAND, PCD_CALCCRC);

    i = 0x7FF;
    do
    {
        n = RC522_ReadReg(MFRC522_REG_DIV_IRQ);
        i--;
    } while ((i != 0) && !(n & 0x04));

    pOutData[0] = RC522_ReadReg(MFRC522_REG_CRC_RESULT_L);
    pOutData[1] = RC522_ReadReg(MFRC522_REG_CRC_RESULT_M);
}


void RC522_Halt(void)
{
    uint8_t buf[4];
    uint16_t backLen;

    buf[0] = 0x50;
    buf[1] = 0x00;
    RC522_CalculateCRC(buf, 2, &buf[2]);
    RC522_ComMF522(PCD_TRANSCEIVE, buf, 4, buf, &backLen);
}
