#include "debug.h"

vu8 val;

// R  - T1CH1 - PC6
// G  - T1CH2 - PC7
// B  - T1CH3 - PC0
// W  - T1CH4 - PD3
// WW - T2CH2 - PD4
// TX - PD5
// RX - PD6

uint8_t uartRxBuf[64];
uint16_t uartRxBufReadIdx = 0;

void USARTx_CFG(void) {
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);

    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_Cmd(DMA1_Channel4, ENABLE); /* USART1 Tx */
    DMA_Cmd(DMA1_Channel5, ENABLE); /* USART1 Rx */

    USART_Cmd(USART1, ENABLE);
}

/*********************************************************************
 * @fn      DMA_INIT
 *
 * @brief   Configures the DMA for USART1.
 *
 * @return  none
 */
void DMA_CFG(void) {
    DMA_InitTypeDef DMA_InitStructure = {0};
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    // DMA_DeInit(DMA1_Channel4);
    // DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DATAR);
    // DMA_InitStructure.DMA_MemoryBaseAddr = 0;
    // DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    // DMA_InitStructure.DMA_BufferSize = 0;
    // DMA_Init(DMA1_Channel4, &DMA_InitStructure);

    DMA_DeInit(DMA1_Channel5);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DATAR);
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)uartRxBuf;
    DMA_InitStructure.DMA_BufferSize = sizeof(uartRxBuf);
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_Init(DMA1_Channel5, &DMA_InitStructure);

    NVIC_InitTypeDef NVIC_InitStructure = {0};
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC | DMA_IT_HT, ENABLE);
}

/*********************************************************************
 * @fn      TIM1_OutCompare_Init
 *
 * @brief   Initializes TIM1 output compare.
 *
 * @param   arr - the period value.
 *          psc - the prescaler value.
 *          ccp - the pulse value.
 *
 * @return  none
 */
void TIM1_PWMOut_Init(u16 arr, u16 psc, u16 ccp) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM1, ENABLE); // CH1-4: PC6, PC7, PC0, PD3

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_TIM1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    TIM_TimeBaseInitStructure.TIM_Period = arr;
    TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

#if (PWM_MODE == PWM_MODE1)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;

#elif (PWM_MODE == PWM_MODE2)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;

#endif

    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = ccp;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

#define ESIG_UNIID ((uint32_t *)0x1FFFF7E8)

int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    // USART_Printf_Init(115200);

    DMA_CFG(); // needs to be first, so DMA cmd enable in next function actually does something
    USARTx_CFG();
    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
    TIM1_PWMOut_Init((1 << 14) - 2, 0, 1);

    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("Chip ID: %08lX %08lX\n", ESIG_UNIID[0], ESIG_UNIID[1]);
    printf("RevID: %04X, DevID: %04x\n", DBGMCU_GetREVID(), DBGMCU_GetDEVID());

    while (1) {}
}

// TODO: don't do that in ISR context
void uartDoDataRead() {
    uint16_t dmaSize = sizeof(uartRxBuf);
    uint16_t dmaPos = dmaSize - DMA_GetCurrDataCounter(DMA1_Channel5); // DMA DataCounter counts backwards
    int bytesToRead = dmaPos - uartRxBufReadIdx;

    if (bytesToRead < 0) { // wrap-around
        bytesToRead = dmaSize - uartRxBufReadIdx;
        fwrite(&uartRxBuf[uartRxBufReadIdx], bytesToRead, 1, stdout);
        fwrite(&uartRxBuf[0], dmaPos, 1, stdout);
    } else {
        fwrite(&uartRxBuf[uartRxBufReadIdx], bytesToRead, 1, stdout);
    }

    // printf("DMA: %2d, IDX: %2d, Read: %2d\n", dmaPos, uartRxBufReadIdx, bytesToRead);
    uartRxBufReadIdx = dmaPos;
}

extern "C" {
// needed for C++ compatibility, apparently
void _fini() {}
void _init() {}

void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel5_IRQHandler() __attribute__((interrupt("WCH-Interrupt-fast")));
}

void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
        // printf("DMA UART RX counter: %d\n", DMA_GetCurrDataCounter(DMA1_Channel5));
        // printf("%s\n", uartRxBuf);
        uartDoDataRead();
        USART_ReceiveData(USART1); // do dummy read to clear IDLE flag
    }
}

void DMA1_Channel5_IRQHandler() {
    if (DMA_GetITStatus(DMA1_IT_TC5) != RESET) {
        // printf("DMA TC IRQ\n");
        uartDoDataRead();
        DMA_ClearITPendingBit(DMA1_IT_TC5);
    }
    else if (DMA_GetITStatus(DMA1_IT_HT5) != RESET) {
        uartDoDataRead();
        DMA_ClearITPendingBit(DMA1_IT_HT5);
    }
}