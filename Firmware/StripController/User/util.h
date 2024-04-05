
#include <ch32v00x.h>
#include <stdio.h>

#if !defined DEBUG_EN
    #define DEBUG_EN 0
#endif

// https://stackoverflow.com/a/1644898
#define printfd(fmt, ...) do { if (DEBUG_EN) fprintf(stderr, fmt, ##__VA_ARGS__); } while (0)


// better delay functions, that keep the SysTick counter running 
// (just make sure not to call the "official" Delay_Ms functions)
void SysTickInit();
uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
void delay_us(uint32_t us);

uint64_t getUID();

#if DEBUG_EN
    void printHex(uint8_t* buf, uint16_t size);
#else
    #define printHex(a,b) (0)
#endif

// Write custom data after the system option bytes
int flashOBWrite(uint8_t *data, size_t size);
// Read custom data after the system option bytes, returns -2 if no valid data was found
int flashOBRead(uint8_t *dst, size_t size);