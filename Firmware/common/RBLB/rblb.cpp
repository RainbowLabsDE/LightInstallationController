#include "rblb.h"
#include "../CRC/crc.h"

#include <stdio.h>
#include <string.h>

void RBLB::loop() {
    #if defined(RBLB_LOWOVERHEAD)
    if (_getCurrentMillis() - _lastByteReceived >= PACKET_TIMEOUT) {
        // discard previous packet if no new bytes arrived for some time
        _curReadIdx = 0;
    }
    #endif
}

void RBLB::handleByte(uint8_t byte) {
    #if !defined(RBLB_LOWOVERHEAD)
    uint32_t curMillis = _getCurrentMillis();
    if (curMillis - _lastByteReceived >= PACKET_TIMEOUT) {
        // discard previous packet if no new bytes arrived for some time
        _curReadIdx = 0;
    }
    #endif

    if (_curReadIdx == 0) {
        // printf("\n");
        if (byte >= DiscoveryInit) {    // uid command
            memset(_packetBuf, 0, sizeof(_packetBuf));
        }
        else if (byte == CMD::DataSimple) {
            if (_dataBuf) {
                memset(_dataBuf, 0, _dataBufSize);
            }
            _dataChkSum = 0;
            _dataChkXor = 0;
        }
        // printf("\n[%016llX] New Packet  ", _uid);
        // reset on first received byte. Relevant for collision detection
        _lastCrcCorrect = false; 
    }

    if (_curReadIdx < sizeof(_packetBuf)) {
        _packetBuf[_curReadIdx] = byte;
    }

    // UID command
    if (_packetBuf[0] != CMD::DataSimple) {    // TODO: longer data won't fit in packetBuf
        // complete header read?
        if (_curReadIdx >= sizeof(uidCommHeader_t)) {
            // complete data read? (need to check this separately, otherwise data length is unknown)
            uidCommHeader_t *header = (uidCommHeader_t*)_packetBuf;
            if (_curReadIdx >= (sizeof(uidCommHeader_t) + header->len + sizeof(uint16_t)) - 1) {

                if (header->len > sizeof(_packetBuf)) {
                    // can't handle oversized packet, don't do anything as of right now.
                    // TODO: skip remaining bytes according to header->len?
                    _curReadIdx = 0;
                    return;
                }


                // printf("Packet: ");
                // for (int i = 0; i <= _curReadIdx; i++) {
                //     printf("%02X ", _packetBuf[i]);
                // }
                // printf("\n");

                // check checksum
                uint16_t crc = crc16(_packetBuf, sizeof(uidCommHeader_t) + header->len);
                uint16_t packetCrc;
                memcpy(&packetCrc, _packetBuf + sizeof(uidCommHeader_t) + header->len, sizeof(uint16_t));

                if (crc == packetCrc) {
                    _lastCrcCorrect = true;
                    // printf("\n[%016llX] Correct CRC  ", _uid);
                    handlePacketInternal(header, header->data);

                    // received correct packet, so we don't need to rely on timeout to detect next packet
                    _curReadIdx = 0;
                }
                else {
                    // printf("\n[%016llX] Wrong CRC  ", _uid);
                    // set wrong CRC flag here again for good measure (collision detection)
                    _lastCrcCorrect = false;
                    // TODO: also set failed discovery state directly, as to speed up process
                }

                return;
            }
        }
    }
    else {  // data packet
        // TODO: filter out data relevant for oneself and parse it according to the config
        // save into buffer and only commit once datastream ends / crc/length correct for synchronization purposes
        if (_curReadIdx >= sizeof(simpleCommHeader_t)) {
            int dataIdx = _curReadIdx - sizeof(simpleCommHeader_t) - _dataStart;
            
            if (dataIdx >= 0 && dataIdx < _dataLen) {
                _dataBuf[dataIdx] = byte;
            }
            _dataChkSum += byte;
            _dataChkXor ^= byte;

            simpleCommHeader_t *header = (simpleCommHeader_t*)_packetBuf;

            // all data received, according to header
            if (_curReadIdx >= sizeof(simpleCommHeader_t) + header->len - 1) {
                if (header->chkSum == _dataChkSum && header->chkXor == _dataChkXor) {
                    if (_dataCallback) {
                        _dataCallback(_dataBuf, _dataLen);

                        // received correct packet, so we don't need to rely on timeout to detect next packet
                        _curReadIdx = 0;
                        return;
                    }
                }
            }
        }
    }


    _curReadIdx++;
    _lastByteReceived = _getCurrentMillis();
}

void RBLB::handlePacketInternal(uidCommHeader_t *header, uint8_t *payload) {
    if (header->address != _uid && header->address != ADDR_BROADCAST) {
        // not addressed to me
        return;
    }

    switch (header->cmd) {
        case DiscoveryInit:
            _discovered = false;
            break;
        case DiscoveryBurst:
            // TODO:
            // Send time staggered? One discovery burst reply takes roughly 130 us @ 1MBaud
            // calculate timeslots based on baud rate, set maximum slots / delay, maybe 1ms? (~7 slots @ 1MBaud)
            if (!_discovered) {
                cmd_discovery_burst_t *disc = (cmd_discovery_burst_t *)header->data;
                // printf(" RXed burst ");
                if (_uid >= disc->searchAddress_min && _uid <= disc->searchAddress_max) {
                    sendPacket(DiscoveryBurst|Response, _uid);   // respond to burst
                }
            }
            break;
        case DiscoverySilence:
            // printf(" Discovered! ");
            sendPacket(DiscoverySilence|Response, _uid); // respond to silence
            _discovered = true;
            break;
        case DiscoveryInit|Response:
        case DiscoveryBurst|Response:
        case DiscoverySilence|Response:
            // ignore own / duplicate messages
            break;
        case Data:
            if (_dataCallback) {
                _dataCallback(payload, header->len);
            }
            break;
        default:
            _packetCallback(header, payload, this);
            break;
    }
}

size_t RBLB::sendPacket(uint8_t cmd, uint64_t dstUid, const uint8_t *payload, size_t payloadSize) {
    uint16_t packetSize = sizeof(uidCommHeader_t) + payloadSize + sizeof(uint16_t);     // header + payload + CRC
    uint8_t buf[packetSize];
    uidCommHeader_t *header = (uidCommHeader_t *)buf;

    header->cmd = cmd;
    header->address = dstUid;
    header->len = payloadSize;
    if (payload != NULL) {
        memcpy(buf + sizeof(uidCommHeader_t), payload, payloadSize);    // copy payload to packet buffer
    }
    uint16_t crc = crc16(buf, sizeof(uidCommHeader_t) + payloadSize);   // calculate CRC and store at end of packet
    memcpy(buf + sizeof(uidCommHeader_t) + payloadSize, &crc, sizeof(uint16_t));

    _sendBytes(buf, packetSize);
    return packetSize;
}
