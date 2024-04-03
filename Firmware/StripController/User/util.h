
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

void printHex(uint8_t* buf, uint16_t size);

// Write custom data after the system option bytes
int flashOBWrite(uint8_t *data, size_t size);
// Read custom data after the system option bytes, returns -2 if no valid data was found
int flashOBRead(uint8_t *dst, size_t size);