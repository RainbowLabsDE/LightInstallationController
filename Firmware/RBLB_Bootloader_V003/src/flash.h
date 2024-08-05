#pragma once
#include "ch32v003fun.h"
#include <string.h>

bool flashStarted = false;

// assuming 64 byte data
bool flashWrite(uint32_t address, const uint8_t *data) {
    if (!flashStarted) {
        flashStarted = true;

        // Unlock flash 
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;

        // For unlocking programming, in general.
        FLASH->MODEKEYR = 0x45670123;
        FLASH->MODEKEYR = 0xCDEF89AB;
    }

    if (FLASH->CTLR & 0x8080) {
        // flash locked
        return false;
    }

    if (address & 63) {
        // address not 64-byte aligned
        return false;
    }

    // Erase Page
	FLASH->CTLR = CR_PAGE_ER;
	FLASH->ADDR = address;
	FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
	while( FLASH->STATR & FLASH_STATR_BSY );  // Takes about 3ms.

    if (*((uint32_t *)address) != 0xFFFFFFFF) {
        // erase failed
        return false;
    }

	// Clear buffer and prep for flashing.
	FLASH->CTLR = CR_PAGE_PG;  // synonym of FTPG.
	FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
	FLASH->ADDR = address;

    // Note: It takes about 6 clock cycles for this to finish.
	while( FLASH->STATR & FLASH_STATR_BSY );  // No real need for this.

    for (int i = 0; i < 16; i++) {
        uint32_t u32;
        memcpy(&u32, data + i*4, sizeof(uint32_t));
        *(uint32_t*)address = u32;  // Write to the buffer
		FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD; // Load the buffer.
		while( FLASH->STATR & FLASH_STATR_BSY );  // Only needed if running from RAM.
    }

    // Actually write the flash out. (Takes about 3ms)
	FLASH->CTLR = CR_PAGE_PG|CR_STRT_Set;
	while( FLASH->STATR & FLASH_STATR_BSY );

    for (int i = 0; i < 16; i++) {
        uint32_t *dst = (uint32_t*)address;
        uint32_t *src = (uint32_t*)data;

        if (dst[i] != src[i]) {
            // Data in flash mismatches
            return false;
        }
    }
    return true;
}