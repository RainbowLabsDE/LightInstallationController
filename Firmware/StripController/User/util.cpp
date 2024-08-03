#include "util.h"

#include <string.h>

#define SysTick_CTLR_STE    (1 << 0)    // counter enable
#define SysTick_CTLR_STIE   (1 << 1)    // interrupt enable
#define SysTick_CTLR_STRE   (1 << 3)    // auto-reload enable
#define SysTick_CTLR_SWIE   (1 << 31)   // software interrupt trigger enable

const uint8_t ticksPerUs = SystemCoreClock / 8 / 1000000;
const uint16_t ticksPerMs = ticksPerUs * 1000;
volatile uint32_t _millis = 0;

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

const uint32_t divisionShift = 22;                        // maximum expected result is 1000, so 22 bit remain for the "divisor"
const uint32_t divisionFactor = (1 << 22) / ticksPerUs;   // do micros() division by multiplying and shifting, result should be accurate to 0.001
uint32_t micros() {
    // retVal = millis / 1000 + SysTickCNT / ticksPerUs, but optimized
    uint32_t retVal = -(_millis << 4) - (_millis << 3);
    retVal += (_millis << 9);
    retVal += (_millis << 9); // do += millis<<10 in two steps to avoid overflow
    retVal += (SysTick->CNT * divisionFactor) >> divisionShift; // divide by ticksPerUs
    return retVal;
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


#define ESIG_UNIID ((uint32_t *)0x1FFFF7E8)

uint64_t getUID() {
    uint64_t uid = *(ESIG_UNIID + 1);
    uid <<= 32;
    uid |= *ESIG_UNIID;
    return uid;
}

#if DEBUG_EN
void printHex(uint8_t* buf, uint16_t size) {
	printfd("       ");
	for(uint8_t i = 0; i < 16; i++) {
		printfd("%1X  ", i);
	}
	printfd("\n%04X  ", 0);
	for(uint16_t i = 0; i < size; i++) {
		printfd("%02X ", buf[i]);
		if(i % 16 == 15) {
			printfd("\n%04X  ", i+1);
		}
	}
	printfd("\n");
}
#endif

const uint32_t ProgramTimeout = 0x00002000;
const uint32_t FLASH_KEY1 =     0x45670123;
const uint32_t FLASH_KEY2 =     0xCDEF89AB;
const uint32_t FLASH_CTLR_OBG = 1 << 4;     // Perform user-selected word programming 
const size_t   SYSTEM_OB_NUM =  8;          // Number of system option bytes (byte + inverse byte)

int flashOBWrite(uint8_t *data, size_t size) {
    if (size >= 24) {
        return -1;  // too long to fit into OB area
    }

    FLASH_Status status = FLASH_COMPLETE;
    uint16_t prevOB[SYSTEM_OB_NUM] = {0};
    memcpy(prevOB, OB, sizeof(prevOB));
    printHex((uint8_t*)prevOB, sizeof(prevOB));
    
    FLASH_Unlock();
    FLASH_EraseOptionBytes();
    status = FLASH_WaitForLastOperation(ProgramTimeout);
    if (status == FLASH_COMPLETE)
    {
        // unlock 
        FLASH->OBKEYR = FLASH_KEY1;     // unlock option byte flash area
        FLASH->OBKEYR = FLASH_KEY2;
        FLASH->CTLR |= FLASH_CTLR_OBG;  // set option byte programming mode

        status = FLASH_WaitForLastOperation(ProgramTimeout);
        if (status != FLASH_COMPLETE) {
            return -3;
        }


        // Restore previous system option byte content
        for (size_t i = 0; i < SYSTEM_OB_NUM; i++) {
            *((__IO uint16_t*)(OB_BASE + i*2)) = prevOB[i] & 0xFF;
            status = FLASH_WaitForLastOperation(ProgramTimeout);
            if (status != FLASH_COMPLETE) {
                if (status != FLASH_TIMEOUT) {
                    FLASH->CTLR &= ~FLASH_CTLR_OBG; // turn off option byte programming mode
                }
                FLASH_Lock();
                return -2;
            }
        }

        const int obOffset = SYSTEM_OB_NUM * sizeof(uint16_t);    // custom option bytes offset
        for (size_t i = 0; i < size; i++) {
            *((__IO uint16_t*)(OB_BASE + obOffset + i*2)) = data[i];

            status = FLASH_WaitForLastOperation(ProgramTimeout);
            if (status != FLASH_COMPLETE) {
                if (status != FLASH_TIMEOUT) {
                    FLASH->CTLR &= ~FLASH_CTLR_OBG; // turn off option byte programming mode
                }
                FLASH_Lock();
                return -2;
            }
        }

        if (status != FLASH_TIMEOUT) {
            FLASH->CTLR &= ~FLASH_CTLR_OBG; // turn off option byte programming mode
        }
    }

    FLASH_Lock();

    return 0;
}

int flashOBRead(uint8_t *dst, size_t size) {
    if (size > 24) {
        return -1;  // too long
    }

    uint8_t buf[size];
    
    for (size_t i = 0; i < size; i++) {
        uint16_t bytePair = ((uint16_t*)OB_BASE)[SYSTEM_OB_NUM + i];
        if (((bytePair & 0xFF) ^ (bytePair >> 8)) == 0xFF) {  // check that the inverse "checksum" is correct
            buf[i] = bytePair & 0xFF;
        }
        else {
            return -2;  // byte pair "checksum" missmatch
        }
    }

    memcpy(dst, buf, size);
    return 0;
}