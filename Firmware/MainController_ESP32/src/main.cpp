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

const uint64_t nodeOrder[] = {
    0x4912BC4EE0F7ABCD,
    0x4913BC4EE0F8ABCD,
    0x4915BC4EE0FAABCD,
    0x4916BC4EE0FBABCD,
};
const int nodeOrder_num = sizeof(nodeOrder)/sizeof(nodeOrder[0]);


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

void incomingPacket(RBLB::uidCommHeader_t *header, uint8_t *payload, RBLB* rblbInst) {
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

    pinMode(PIN_RS485_DE, OUTPUT);

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
uint64_t color = 0xFF0000000000;
uint32_t iter = 0;
bool stopped = false;

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


    if (stopped) {
        return;
    }

    if (!discoveryDone) {
        int ret = rblb.discoverNext();
        if (ret > -1) {
            discoveryDone = true;
            printf("\n Found %d UIDs:\n", ret);
            for (int i = 0; i < ret; i++) {
                printf("%016llX\n", discoveredUids[i]);
            }
            if (ret == 0) { stopped = true; }

            // RBLB::cmd_param_t param = {.paramId = RBLB::ParamID::BitsPerColor_Data, .u8 = 8};
            // rblb.sendPacket(RBLB::CMD::SetParameters, RBLB::ADDR_BROADCAST, (uint8_t*)&param, sizeof(RBLB::cmd_param_t));
            // param.paramId = RBLB::ParamID::NodeNum;
            // param.u16 = 1;
            // rblb.sendPacket(RBLB::CMD::SetParameters, RBLB::ADDR_BROADCAST, (uint8_t*)&param, sizeof(RBLB::cmd_param_t));
            
            // RBLB::cmd_param_t param = {.paramId = RBLB::ParamID::NumLEDs, .u16 = 100};
            RBLB::cmd_param_t param = {.paramId = RBLB::ParamID::BitsPerColor_Data, .u8 = 16};
            rblb.sendPacket(RBLB::CMD::SetParameters, RBLB::ADDR_BROADCAST, (uint8_t*)&param, sizeof(RBLB::cmd_param_t));

            param.paramId = RBLB::ParamID::NodeNum;
            for (int i = 0; i < nodeOrder_num; i++) {
                param.u16 = i;
                rblb.sendPacket(RBLB::CMD::SetParameters, nodeOrder[i], (uint8_t*)&param, sizeof(RBLB::cmd_param_t));
            }
        }
        delay(1);
    }

    if (discoveryDone) {
        // printf("Sending color 0x%06llX to %016llX\n", color, discoveredUids[0]);
        // rblb.sendPacket(RBLB::CMD::Data, discoveredUids[0], (uint8_t*)&color, 6);
        // rblb.sendSimpleData((uint8_t*)&color, 6);

        // uint8_t buf[1500] = {0};
        // memcpy(buf + 6, &color, 6);
        // rblb.sendSimpleData((uint8_t*)&buf, sizeof(buf));

        // color += 16;
        // if (color & (0xFFFFULL << 32)) {
        //     color &= 0xFFFF00000000ULL;
        //     color += 0x1000000000ULL;
        // }
        // else if (color & (0xFFFFULL << 16)) {
        //     color &= 0x0000FFFF0000ULL;
        //     color += 0x100000;
        // }
        // else {
        //     color &= 0x00000000FFFF;
        // }
        // delayMicroseconds(2000);

        // uint8_t buf[4*3] = {0};
        // for (int i = 0; i < sizeof(buf); i += 3) {
        //     uint32_t col = 1 << ((i / 3 + millis() / 50) % 24);
        //     memcpy(buf + i, &col, 3);
        // }
        // rblb.sendSimpleData((uint8_t*)&buf, sizeof(buf));

        uint8_t buf[4*6] = {0};
        for (int i = 0; i < sizeof(buf); i += 6) {
            uint64_t col = 1ULL << ((i / 6 + millis() / 200) % 48);
            memcpy(buf + i, &col, 6);
        }
        rblb.sendSimpleData((uint8_t*)&buf, sizeof(buf));

        delayMicroseconds(2000);
        // iter++;
        // if (iter >= 100) {
        //     iter = 0;
        //     delayMicroseconds(2000); // resync
        // }
    }

}