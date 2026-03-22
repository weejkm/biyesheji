#include "SU.h"
#include "stm32f10x.h"
#include "USART.h"

volatile uint8_t SU_data = 0;
volatile uint8_t SU_Receive_flag = 0;

static volatile uint8_t su_rx_buf[4];
static volatile uint8_t su_rx_index = 0;

void SU_Init(void)
{
    USART2_Init(115200);
    USART2_RX_CNT = 0;

    SU_data = 0;
    SU_Receive_flag = 0;
    su_rx_index = 0;
}

void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)
    {
        uint8_t data = (uint8_t)(USART_ReceiveData(USART2) & 0xFF);

        su_rx_buf[su_rx_index++] = data;

        if (su_rx_index >= 4)
        {
            su_rx_index = 0;

            /* 校验固定帧格式：00 00 00 CMD */
            if (su_rx_buf[0] == 0x00 &&
                su_rx_buf[1] == 0x00 &&
                su_rx_buf[2] == 0x00)
            {
                if (SU_Receive_flag == 0)
                {
                    SU_data = su_rx_buf[3];
                    SU_Receive_flag = 1;
                }
            }
        }

        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}
