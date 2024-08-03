#include "uart.h"
#include "ch32v00x_conf.h"
#include <string.h>

UART uart1;

extern "C" {
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel5_IRQHandler() __attribute__((interrupt("WCH-Interrupt-fast")));
}

void USART1_IRQHandler(void) {
    if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET) {
        uart1._isrCallback();
        USART_ReceiveData(USART1); // do dummy read to clear IDLE flag
    }
}

void DMA1_Channel5_IRQHandler() {
    if (DMA_GetITStatus(DMA1_IT_TC5) != RESET) {
        uart1._isrCallback();
        DMA_ClearITPendingBit(DMA1_IT_TC5);
    }
    else if (DMA_GetITStatus(DMA1_IT_HT5) != RESET) {
        uart1._isrCallback();
        DMA_ClearITPendingBit(DMA1_IT_HT5);
    }
}

void UART::initGpio() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);

    // USART1 TX: PD5, RX: PD6
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
}

void UART::initUart() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    USART_InitTypeDef USART_InitStructure = {0};
    USART_InitStructure.USART_BaudRate = 1000000;
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

    USART_Cmd(USART1, ENABLE);
}

void UART::initDma() {
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
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)_rxBuf;
    DMA_InitStructure.DMA_BufferSize = sizeof(_rxBuf);
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

    DMA_Cmd(DMA1_Channel4, ENABLE); /* USART1 Tx */
    DMA_Cmd(DMA1_Channel5, ENABLE); /* USART1 Rx */

    USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
}

UART::UART() {}

void UART::init() {
    initGpio();
    initUart();
    initDma();
}

void UART::_isrCallback() {}

int UART::available() {
    uint16_t dmaSize = sizeof(_rxBuf);
    uint16_t dmaPos = dmaSize - DMA_GetCurrDataCounter(DMA1_Channel5); // DMA DataCounter counts backwards
    int bytesToRead = dmaPos - _rxBufIdx;
    if (bytesToRead < 0) {
        bytesToRead += dmaSize;
    }
    return bytesToRead;
}

uint8_t UART::read() {
    uint8_t val = _rxBuf[_rxBufIdx];
    _rxBufIdx++;
    if (_rxBufIdx >= sizeof(_rxBuf)) {
        _rxBufIdx = 0;
    }
    return val;
}

size_t UART::readBytes(uint8_t *buf, size_t size) {
    uint16_t dmaSize = sizeof(_rxBuf);
    uint16_t dmaPos = dmaSize - DMA_GetCurrDataCounter(DMA1_Channel5); // DMA DataCounter counts backwards
    int bytesToRead = dmaPos - _rxBufIdx;

    if (bytesToRead < 0) {  // data in buffer wraps around
        // bytesToRead += dmaSize;
        size_t numBytesEnd = dmaSize - _rxBufIdx;    // number of bytes at the end of the buffer
        if (numBytesEnd > size) {    // not enough room in destination
            bytesToRead = size;
            // no wrap-around needed, fall through to below for normal copy
        }
        else {
            memcpy(buf, _rxBuf + _rxBufIdx, numBytesEnd);    // read remainder of buffer into destination
            _rxBufIdx = 0;
            bytesToRead = dmaPos + numBytesEnd;
            size_t numBytesFront = dmaPos;          // number of bytes wrapped around to beginning of buffer
            if (bytesToRead > (int)size) {
                bytesToRead = size;
                numBytesFront = size - numBytesEnd;
            }
            memcpy(buf + numBytesEnd, _rxBuf, numBytesFront);
            _rxBufIdx = numBytesFront;
            return bytesToRead;
        }
    }
    if (bytesToRead > 0) {
        if (bytesToRead > (int)size) {   // clamp to max destination buffer size
            bytesToRead = size;
        }
        memcpy(buf, _rxBuf + _rxBufIdx, bytesToRead);
        _rxBufIdx += bytesToRead;
    }
    else {
        return 0;
    }

    return bytesToRead;
}

size_t UART::sendBytes(const uint8_t *buf, size_t size) {
    // For now, just do it blockingly, TODO: make faster?
    for (volatile size_t i = 0; i < size; i++) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
        USART_SendData(USART1, buf[i]);
    }
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);    // need blocking for correct RS485 timing.
    return size;
}
