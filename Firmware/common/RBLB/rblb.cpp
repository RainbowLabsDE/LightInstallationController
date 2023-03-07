#include "rblb.h"

#include <stdio.h>
#include <string.h>

RBLB::RBLB(uint64_t uid) {
    _uid = uid;
}

void RBLB::handleByte(uint8_t byte) {
    // TODO: detect end of transmission

    if (_curReadIdx == 0) {
        memset(_packetBuf, 0, sizeof(_packetBuf));
        if (byte >= DiscoveryInit) {    // uid command
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
                // TODO: check checksum
                // TODO: handle command packet

                printf("Packet: ");
                for (int i = 0; i <= _curReadIdx; i++) {
                    printf("%02X ", _packetBuf[i]);
                }
                printf("\n");

                // received correct packet, so we don't need to rely on timeout to detect next packet
                _curReadIdx = 0;
                return;
            }
        }
    }
    else {  // data packet
        // TODO: filter out data relevant for oneself and parse it according to the config

    }


    _curReadIdx++;
    // save current time for timeout variable?
}