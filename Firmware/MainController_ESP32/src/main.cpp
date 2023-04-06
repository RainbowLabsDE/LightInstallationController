#include <Arduino.h>
#include "../../common/RBLB/rblb.h"

const unsigned PIN_TX = 26;
const unsigned PIN_RX = 18;

void rs485Write(const uint8_t *buf, size_t size) {
    // digitalWrite(PIN_RS485_DE, HIGH);
    Serial1.write(buf, size);
    Serial1.flush();
    // delayMicroseconds(15000000 / rs485.baud());   // delay needed, otherwise last byte is cut off and flush() doesn't do enough
    // digitalWrite(PIN_RS485_DE, LOW);
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

RBLB rblb(0, rs485Write, incomingPacket, (uint32_t (*)())millis);
size_t bytesToIgnore = 0;

void setup() {
    delay(2500);
    Serial1.begin(1000000, SERIAL_8N1, PIN_RX, PIN_TX);
    Serial.begin(2000000);
    printf("\nRainbowLabs LIC MainController ESP32 - Hello World!\n");

    // Serial1.print("Hello World.");
}

uint32_t lastByteRxd = 0;
uint32_t lastPacketSent = 0;

void loop() {
    while (Serial1.available()) {
        char c = Serial1.read();

        if (millis() - lastByteRxd > 3) {
            printf("\n");
        }
        printf("%02X ", c);
        fflush(stdout);
        lastByteRxd = millis();

        if (bytesToIgnore) {
            bytesToIgnore--;
            continue;
        }
        rblb.handleByte(c);
    }

    if (millis() - lastPacketSent > 1000) {
        bytesToIgnore = rblb.sendPacket(RBLB::GetStatus, RBLB::ADDR_BROADCAST);
        lastPacketSent = millis();
    }
}