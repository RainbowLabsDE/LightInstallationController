#include "ch32v003fun.h"
#include "crc.h"
#include "globals.h"
#include "flash.h"
#include "rblb.h"

#pragma pack(push, 1)
// lives at last bytes of application flash (TODO)
typedef struct {
    char date[12];  // Compile date
    char time[9];   // Compile time
    uint16_t crc;   // crc of whole flash excluding this section
} flashMarker_t;


// typedef struct {
//     uint8_t cmd;
//     uint64_t uid;
//     uint16_t crc;
//     uint32_t address;
//     uint8_t data[64];
// } blPacket_t;
#pragma pack(pop)

// enum {
//     CMD_FLASH = 0x70
// };

uint8_t uartBuf[sizeof(RBLB::uidCommHeader_t) + sizeof(RBLB::cmd_flashPage_t) + sizeof(uint16_t)];

uint32_t lastByteReceived = 0; // in SysTickCounts

int uartIdx = 0;

uint16_t completeCrc = 0, runningCrc = 0;

#define BL_UART_TIMEOUT (2 * DELAY_MS_TIME)

void uartBusDominant() {
    funDigitalWrite(PIN_RS485_DE, 1);
    funDigitalWrite(PIN_TX, 0);
}

void uartBusFree() {
    funDigitalWrite(PIN_TX, 1);
    funDigitalWrite(PIN_RS485_DE, 0);
}

void uartHandle() {
    RBLB::uidCommHeader_t *header = (RBLB::uidCommHeader_t*)uartBuf;
    uint16_t pktCrc = *(uint16_t*)(uartBuf + sizeof(RBLB::uidCommHeader_t) + header->len);  // get CRC at end of packet
    uint16_t calcCrc = crc16(uartBuf, sizeof(RBLB::uidCommHeader_t) + header->len);
    
    // CRC correct and me/broadcast is addressed
    if (pktCrc == calcCrc && (header->address == UINT64_MAX || header->address == MY_UID)) {
        switch (header->cmd) {
            case RBLB::FlashStart: {
                RBLB::cmd_flashPage_t *page = (RBLB::cmd_flashPage_t*)header->data;
                completeCrc = page->completeCrc;
                runningCrc = 0;
                break;
            }
            case RBLB::FlashPage: {
                RBLB::cmd_flashPage_t *page = (RBLB::cmd_flashPage_t*)header->data;
                uartBusDominant();
                flashWrite(page->address, page->data);
                update_crc16(&runningCrc, page->data, 64);
                uartBusFree();
                break;
            }
            case RBLB::FlashDone: {
                if (runningCrc != completeCrc) {
                    // well, fuck
                }
                break;
            }
        }
    }
}

void uartLoop() {
    if (USART1->STATR & USART_STATR_RXNE) {
        // Reset index back to beginning, when new packet starts
        if (SysTick->CNT - lastByteReceived > BL_UART_TIMEOUT) {
            uartIdx = 0;
        }

        if (uartIdx < sizeof(uartBuf)) {
            uartBuf[uartIdx] = USART1->DATAR;
            uartIdx++;
        }
        lastByteReceived = SysTick->CNT;
    }

    // Simply check if complete packet got received by length (only works for a single command)
    if (uartIdx == sizeof(uartBuf)) {
        uartHandle();
        uartIdx = 0;
    }
}