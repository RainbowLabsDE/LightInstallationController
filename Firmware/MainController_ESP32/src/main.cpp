#include <Arduino.h>
#include "../../common/RBLB/rblb_host.h"

const unsigned PIN_TX = 26;
const unsigned PIN_RX = 18;
const unsigned PIN_RS485_DE = 33;


const unsigned discoveredUidsSize = 128;
uint64_t discoveredUids[discoveredUidsSize];
// const size_t numMockNodes = 2;
// RBLB *mockNodes[numMockNodes];

// const size_t mockSerialBufSize = 128;
// char mockSerialBuf[mockSerialBufSize];
// int mockSerialBufIdx = 0;


void rs485Write(const uint8_t *buf, size_t size) {
    digitalWrite(PIN_RS485_DE, HIGH);
    Serial1.write(buf, size);
    Serial1.flush();
    // delayMicroseconds(15000000 / rs485.baud());   // delay needed, otherwise last byte is cut off and flush() doesn't do enough
    digitalWrite(PIN_RS485_DE, LOW);

    // printf("\n[H] > ");
    // for (int i = 0; i < size; i++) {
    //     printf("%02X ", buf[i]);
    //     for (int m = 0; m < numMockNodes; m++) {
    //         mockNodes[m]->handleByte(buf[i]);
    //     }
    // }
}

void incomingPacket(RBLB::uidCommHeader_t *header, uint8_t *payload) {
    printf("RBLB packet: cmd: %02X, addr: %016llX, len: %4d", header->cmd, header->address, header->len);
    if (header->len) {
        printf(", payload: ");
    }
    for (int i = 0; i < header->len; i++) {
        printf("%02X ", payload[i]);
    }
    printf("\n");
}



void mockWrite(const uint8_t *buf, size_t size, uint64_t id) {
    printf("\n[%016llX] > ", id);
    // for (int i = 0; i < size; i++) {
    //     printf("%02X ", buf[i]);
    //     // rblb.handleByte(buf[i]);
    // }
    // printf("\n");
    rs485Write(buf, size);
}
// void mockWrite1(const uint8_t *buf, size_t size) { mockWrite(buf, size, 0x8851BC48203C0001); };
// void mockWrite2(const uint8_t *buf, size_t size) { mockWrite(buf, size, 0x8851BC48203C0002); };
// void mockWriteH(const uint8_t *buf, size_t size) { mockWrite(buf, size, 0); };
RBLB_Host rblb(0, rs485Write, incomingPacket, (uint32_t (*)())millis);
size_t bytesToIgnore = 0;

void setup() {
    // mockNodes[0] = new RBLB(0x8851BC48203C0001, mockWrite1, incomingPacket, (uint32_t (*)())millis);
    // mockNodes[1] = new RBLB(0x8851BC48203C0002, mockWrite2, incomingPacket, (uint32_t (*)())millis);

    delay(250);
    Serial1.begin(1000000, SERIAL_8N1, PIN_RX, PIN_TX);
    Serial.begin(2000000);
    printf("\nRainbowLabs LIC MainController ESP32 - Hello World!\n");

    // Serial1.print("Hello World.");
    rblb.discoveryInit(discoveredUids, discoveredUidsSize);
}

uint32_t lastByteRxd = 0;
uint32_t lastPacketSent = 0;
bool discoveryDone = false;

void loop() {
    while (Serial1.available()) {
        char c = Serial1.read();

        // if (millis() - lastByteRxd > 3) {
        //     printf("\n    ");
        // }
        printf("%02X ", c);
        fflush(stdout);
        lastByteRxd = millis();

        if (bytesToIgnore) {
            bytesToIgnore--;
            continue;
        }
        rblb.handleByte(c);
        // for (int m = 0; m < numMockNodes; m++) {
        //     mockNodes[m]->handleByte(c);
        // }
    }

    // if (millis() - lastPacketSent > 1000) {
    //     bytesToIgnore = rblb.sendPacket(RBLB::GetStatus, RBLB::ADDR_BROADCAST);
    //     lastPacketSent = millis();
    // }
    if (!discoveryDone) {
        int ret = rblb.discoverNext();
        if (ret > -1) {
            discoveryDone = true;
            printf("\n Found %d UIDs:\n", ret);
            for (int i = 0; i < ret; i++) {
                printf("%016llX\n", discoveredUids[i]);
            }
        }
    }
    delay(1);
}