#include "rblb.h"
#include "../CRC/crc.h"

#include <stdio.h>
#include <string.h>

RBLB::RBLB(uint64_t uid,
        void (*txFunc)(const uint8_t *buf, size_t size),
        void (*packetCallback)(uidCommHeader_t *header, uint8_t *payload),
        uint32_t (*getCurrentMillis)()) {
    _uid = uid;
    _sendBytes = txFunc;
    _packetCallback = packetCallback;
    _getCurrentMillis = getCurrentMillis;
}

void RBLB::handleByte(uint8_t byte) {
    // TODO: detect end of transmission (timeout?)

    if (_curReadIdx == 0) {
        if (byte >= DiscoveryInit) {    // uid command
            memset(_packetBuf, 0, sizeof(_packetBuf));
        }
    }

    if (_curReadIdx < sizeof(_packetBuf)) {
        _packetBuf[_curReadIdx] = byte;
    }

    // UID command
    if (_packetBuf[0] >= DiscoveryInit) {
        // complete header read?
        if (_curReadIdx >= sizeof(uidCommHeader_t)) {
            // complete data read? (need to check this separately, otherwise data length is unknown)
            uidCommHeader_t *header = (uidCommHeader_t*)_packetBuf;
            if (_curReadIdx >= sizeof(uidCommHeader_t) + header->len + 2) {


                printf("Packet: ");
                for (int i = 0; i <= _curReadIdx; i++) {
                    printf("%02X ", _packetBuf[i]);
                }
                printf("\n");

                // check checksum
                uint16_t crc = crc16(_packetBuf, sizeof(uidCommHeader_t) + header->len);
                uint16_t packetCrc;
                memcpy(&packetCrc, _packetBuf + sizeof(uidCommHeader_t) + header->len, 2);

                if (crc == packetCrc) {
                    handlePacketInternal(header, header->data);
                }
                else {
                    printf("Wrong CRC\n");
                }


                // received correct packet, so we don't need to rely on timeout to detect next packet
                _curReadIdx = 0;
                return;
            }
        }
    }
    else {  // data packet
        // TODO: filter out data relevant for oneself and parse it according to the config
        // save into buffer and only commit once datastream ends / crc/length correct for synchronization purposes
    }


    _curReadIdx++;
    // TODO: save current time for timeout variable?
}

void RBLB::handlePacketInternal(uidCommHeader_t *header, uint8_t *payload) {
    if (header->address != _uid || header->address != ADDR_BROADCAST) {
        // not addressed to me
        return;
    }

    switch (header->cmd) {
        case DiscoveryInit:
            _discovered = false;
            break;
        case DiscoveryBurst:
            // TODO
            // Send time staggered? One discovery burst reply takes roughly 130 us @ 1MBaud
            // calculate timeslots based on baud rate, set maximum slots / delay, maybe 1ms? (~7 slots @ 1MBaud)
            break;
        case DiscoverySilence:
            _discovered = true;
            break;
        default:
            _packetCallback(header, payload);
            break;
    }
}