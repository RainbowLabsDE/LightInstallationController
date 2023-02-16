
#include <ch32v00x.h>
#include <stdio.h>

// better delay functions, that keep the SysTick counter running 
// (just make sure not to call the "official" Delay_Ms functions)
void SysTickInit();
uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
void delay_us(uint32_t us);

uint64_t getUID();