#include "util.h"

#define SysTick_CTLR_STE    (1 << 0)    // counter enable
#define SysTick_CTLR_STIE   (1 << 1)    // interrupt enable
#define SysTick_CTLR_STRE   (1 << 3)    // auto-reload enable
#define SysTick_CTLR_SWIE   (1 << 31)   // software interrupt trigger enable

const uint8_t ticksPerUs = SystemCoreClock / 8 / 1000000;
const uint16_t ticksPerMs = ticksPerUs * 1000;
uint32_t _millis = 0;

void SysTickInit() {
    SysTick->CTLR &= SysTick_CLKSource_HCLK_Div8;
    SysTick->CTLR |= SysTick_CTLR_STIE;             // enable interrupt
    SysTick->CTLR |= SysTick_CTLR_STRE;             // enable auto-reload
    SysTick->SR &= ~(1 << 0);                       // clear count value comparison flag
    SysTick->CNT = 0;                               // reset counter
    SysTick->CMP = ticksPerMs - 1;                  // set compare to 1ms

    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CTLR |= SysTick_CTLR_STE;              // enable counter
}

// gets called every ms
extern "C" {
    void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
}
void SysTick_Handler(void) {
    _millis++;
    SysTick->SR = 0;
}

uint32_t millis() {
    return _millis;
}

uint32_t micros() {
    return (_millis * 1000) + (SysTick->CNT / ticksPerUs);
}

void delay(uint32_t ms) {
    uint32_t start = micros();
    if (ms < 60000) {   // under 1 min, use micros for higher accuracy (?)
        uint32_t us = ms * 1000;
        while (micros() - start < us);
    }
    else {
        start = millis();
        while (millis() - start < ms);
    }
}

void delay_us(uint32_t us) {
    if (us < 1000) {    // under 1ms, use SysTick directly, TODO: check if actually more accurate
        uint32_t start = SysTick->CNT;
        uint32_t ticks = us * ticksPerUs;
        while ((SysTick->CNT - start) % (1000 * ticksPerUs) < ticks);
    }
    else {
        uint32_t start = micros();
        while (micros() - start < us);
    }
}
