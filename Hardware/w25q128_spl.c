#include "w25q128_spl.h"
#include "delay.h"



/* ===================== SPI 基础收发 ===================== */
/**
 * @brief 通过 SPI2 发送并接收一个字节
 * @param data 待发送的数据
 * @return 接收到的数据
 */
static uint8_t SPI2_TransferByte(uint8_t data)
{
    /* 等待发送缓冲区空 */
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
    /* 发送数据 */
    SPI_I2S_SendData(SPI2, data);
    /* 等待接收缓冲区满 */
    while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
    /* 返回接收到的数据 */
    return (uint8_t)SPI_I2S_ReceiveData(SPI2);
}

/* ===================== 外设与 GPIO 初始化 ===================== */
/**
 * @brief 初始化 SPI2 和相关 GPIO 引脚，用于驱动 W25Q128
 * 
 * 硬件连接：
 *   PB12 -> CS（片选）
 *   PB13 -> SCK（时钟）
 *   PB14 -> MISO（主机输入，从机输出）
 *   PB15 -> MOSI（主机输出，从机输入）
 */
void W25Q_SPI2_Init(void)
{
    /* 1. 打开外设时钟：GPIOB (APB2) 和 SPI2 (APB1) */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2,  ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;

    /* 配置 SCK (PB13) 为复用推挽输出 */
    gpio.GPIO_Pin   = GPIO_Pin_13;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &gpio);

    /* 配置 MOSI (PB15) 为复用推挽输出 */
    gpio.GPIO_Pin   = GPIO_Pin_15;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &gpio);

    /* 配置 MISO (PB14) 为浮空输入 */
    gpio.GPIO_Pin   = GPIO_Pin_14;
    gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &gpio);

    /* 配置 CS (PB12) 为推挽输出，并默认拉高（禁止选中） */
    gpio.GPIO_Pin   = GPIO_Pin_12;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio);
    W25Q_CS_HIGH();

    /* 2. 配置 SPI2 */
    SPI_InitTypeDef spi;
    SPI_StructInit(&spi);
    spi.SPI_Direction         = SPI_Direction_2Lines_FullDuplex; // 全双工
    spi.SPI_Mode              = SPI_Mode_Master;                 // 主机模式
    spi.SPI_DataSize          = SPI_DataSize_8b;                 // 8位数据
    spi.SPI_CPOL              = SPI_CPOL_Low;                    // 空闲时 SCK 低电平
    spi.SPI_CPHA              = SPI_CPHA_1Edge;                  // 第1个时钟沿采样
    spi.SPI_NSS               = SPI_NSS_Soft;                    // 软件管理片选
    spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;         // 分频 72/8=9MHz
    spi.SPI_FirstBit          = SPI_FirstBit_MSB;                // 高位先行
    spi.SPI_CRCPolynomial     = 7;
    SPI_Init(SPI2, &spi);

    /* 3. 启动 SPI2 */
    SPI_Cmd(SPI2, ENABLE);
}

/* ===================== 基本指令函数 ===================== */

/**
 * @brief 读取状态寄存器1
 * @return 状态寄存器值
 * @note bit0 表示 Busy，bit1 表示 WEL（写使能锁存）
 */
uint8_t W25Q_ReadSR1(void)
{
    uint8_t sr;
    W25Q_CS_LOW();
    SPI2_TransferByte(W25Q_CMD_READ_SR1);   // 发送命令
    sr = SPI2_TransferByte(0xFF);           // 读取状态寄存器
    W25Q_CS_HIGH();
    return sr;
}

/**
 * @brief 发送写使能命令
 */
void W25Q_WriteEnable(void)
{
    W25Q_CS_LOW();
    SPI2_TransferByte(W25Q_CMD_WRITE_ENABLE);
    W25Q_CS_HIGH();
}

/**
 * @brief 等待芯片空闲
 */
void W25Q_WaitBusy(void)
{
    while (W25Q_ReadSR1() & 0x01)   // 判断 Busy 位
    { 
        Delay_us(50);               // 每 50us 轮询一次
    }
}

/* ===================== 读 JEDEC ID ===================== */
/**
 * @brief 读取 JEDEC ID（厂商ID + 器件ID）
 * @return 32位 ID，例如 Winbond W25Q128 = 0xEF4018
 */
uint32_t W25Q_ReadJEDECID(void)
{
    uint32_t id = 0;
    W25Q_CS_LOW();
    SPI2_TransferByte(W25Q_CMD_READ_ID);
    id |= ((uint32_t)SPI2_TransferByte(0xFF)) << 16; // 厂商 ID
    id |= ((uint32_t)SPI2_TransferByte(0xFF)) << 8;  // 存储器类型
    id |= ((uint32_t)SPI2_TransferByte(0xFF));       // 容量
    W25Q_CS_HIGH();
    return id;
}

/* ===================== 数据读取 ===================== */
/**
 * @brief 从指定地址读取数据
 * @param addr 起始地址（24位地址）
 * @param buf  存放数据的缓冲区
 * @param len  要读取的字节数
 */
void W25Q_Read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    W25Q_CS_LOW();
    SPI2_TransferByte(W25Q_CMD_READ_DATA);          // 发送读命令
    SPI2_TransferByte((addr >> 16) & 0xFF);         // 地址高字节
    SPI2_TransferByte((addr >> 8)  & 0xFF);         // 地址中字节
    SPI2_TransferByte(addr & 0xFF);                 // 地址低字节
    while (len--) *buf++ = SPI2_TransferByte(0xFF); // 连续读取
    W25Q_CS_HIGH();
}

/* ===================== 页编程（写入） ===================== */
/**
 * @brief 页编程（最多 256 字节，且不可跨页）
 * @param addr 起始地址（必须在页内）
 * @param buf  待写入的数据缓冲区
 * @param len  写入长度（最大 256）
 */
void W25Q_PageProgram(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    if (len == 0) return;
    if (len > W25Q_PAGE_SIZE) len = W25Q_PAGE_SIZE; // 防止溢出

    W25Q_WriteEnable();  // 先使能写操作

    W25Q_CS_LOW();
    SPI2_TransferByte(W25Q_CMD_PAGE_PROGRAM);       // 页写入命令
    SPI2_TransferByte((addr >> 16) & 0xFF);         // 地址高字节
    SPI2_TransferByte((addr >> 8)  & 0xFF);         // 地址中字节
    SPI2_TransferByte(addr & 0xFF);                 // 地址低字节
    while (len--) SPI2_TransferByte(*buf++);        // 写入数据
    W25Q_CS_HIGH();

    W25Q_WaitBusy(); // 等待写完成
}

/* ===================== 擦除操作 ===================== */
/**
 * @brief 擦除一个扇区（4KB）
 * @param addr 扇区地址（任意地址，内部会按扇区对齐）
 */
void W25Q_SectorErase(uint32_t addr)
{
    W25Q_WriteEnable();
    W25Q_CS_LOW();
    SPI2_TransferByte(W25Q_CMD_SECTOR_ERASE);
    SPI2_TransferByte((addr >> 16) & 0xFF);
    SPI2_TransferByte((addr >> 8)  & 0xFF);
    SPI2_TransferByte(addr & 0xFF);
    W25Q_CS_HIGH();
    W25Q_WaitBusy();
}

/**
 * @brief 擦除一个 64KB 块
 * @param addr 块地址
 */
void W25Q_BlockErase64K(uint32_t addr)
{
    W25Q_WriteEnable();
    W25Q_CS_LOW();
    SPI2_TransferByte(W25Q_CMD_BLOCK_ERASE_64K);
    SPI2_TransferByte((addr >> 16) & 0xFF);
    SPI2_TransferByte((addr >> 8)  & 0xFF);
    SPI2_TransferByte(addr & 0xFF);
    W25Q_CS_HIGH();
    W25Q_WaitBusy();
}

/**
 * @brief 整片擦除（所有数据清 0xFF）
 * @note 耗时最长，可能几十秒
 */
void W25Q_ChipErase(void)
{
    W25Q_WriteEnable();
    W25Q_CS_LOW();
    SPI2_TransferByte(W25Q_CMD_CHIP_ERASE);
    W25Q_CS_HIGH();
    W25Q_WaitBusy();
}
