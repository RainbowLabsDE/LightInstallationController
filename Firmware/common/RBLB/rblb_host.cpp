#if !defined CH32V003   // disable Host functionality for client MCU, TODO: handle more elegantly

#include "rblb_host.h"
#include "../CRC/crc.h"

#include <stdio.h>
#include <string.h>

void RBLB_Host::handlePacketInternal(uidCommHeader_t *header, uint8_t *payload) {
    switch (header->cmd) {
        case DiscoveryBurst|Response:    // valid discovery response received
            if (header->address == ADDR_BROADCAST) {
                break;
            }
            // discoveredValidUid = header->address;
            // TODO: for time staggered replies, need to wait for a bit / need other logic
            sendPacket(DiscoverySilence, header->address);  // try to silence node
            discoveryState = DiscoveryWaitingForSilenceACK;
            // printf("\n[DSC] State: %d", discoveryState);
            break;
        case DiscoverySilence|Response:  // received discovery silence ACK from node (indicating correct address was indeed discovered)
            if (header->address == ADDR_BROADCAST) {
                break;
            }
            // if (header->address == discoveredValidUid) { // if we got valid crc, it ought to be correct :shrug:
                if (_discoveredUids && discoveredUidsNum < _discoveredUidsSize) {
                    _discoveredUids[discoveredUidsNum++] = header->address;
                }
                discoveryState = DiscoveryGotValidUID;
                // printf("\n[DSC] State: %d", discoveryState);
            // }
            // else {
                // also indicates collision, I guess
                // printf("Error: Other node than expected replied to discovery silence. (Expected: %016llX, got: %016llX)\n", discoveredValidUid, header->address);
                // _discoverySilenceCollision = true;
                // discoveryState = 
            // }
            
            break;
        case DiscoveryInit:
        case DiscoveryInit|Response:
        case DiscoveryBurst:
        case DiscoverySilence:
            // ignore own / duplicate messages
        break;
        default:
            _packetCallback(header, payload, this);
            break;
    }
}

void RBLB_Host::discoveryInit(uint64_t *discoveredUids, size_t size) {
    _discoveredUids = discoveredUids;
    _discoveredUidsSize = size;
    sendPacket(DiscoveryInit, ADDR_BROADCAST);
    // TODO: mute already known UIDs
}

int RBLB_Host::discoverNext() {

    if (discoveryStackIdx == 0) {   // new discovery process
        if (discoveryStack) {
            free(discoveryStack);
            discoveryStack = NULL;
        }
        discoveryStack = (discovery_step_t*) malloc(sizeof(discovery_step_t) * discoveryStackSize);     // TODO: can be static once Host/Node instance are split
        discoveryStack[discoveryStackIdx++] = {.baseAddr = 0, .level = 0, .upperHalf = true};
        discoveryStack[discoveryStackIdx++] = {.baseAddr = 0, .level = 0, .upperHalf = false};
        discoveredUidsNum = 0;
    }

    discovery_step_t curStep = discoveryStack[discoveryStackIdx - 1];   // get next step (pop below, only when required)

    uint64_t levelMask = 0x8000'0000'0000'0000 >> curStep.level;    // calculate bit mask for current level of binary tree
    uint64_t addrMin = curStep.baseAddr + (curStep.upperHalf ? levelMask : 0);
    uint64_t addrMax = addrMin + (levelMask - 1);

    // printf("\n[DSC] State: %d, StackIdx: %2d, Level: %2d, Addr: %16llX-%16llX  ", discoveryState, discoveryStackIdx, curStep.level, addrMin, addrMax);

    switch (discoveryState) {
        case DiscoveryIdle: {
            cmd_discovery_burst_t payload = {
                .searchAddress_min = addrMin,
                .searchAddress_max = addrMax
            };
            sendPacket(DiscoveryBurst, ADDR_BROADCAST, (uint8_t *)&payload, sizeof(payload));
            discoveryState = DiscoveryWaitingForResponse;
            _discoverySilenceCollision = false;
            break;
        }
        case DiscoveryWaitingForResponse: {
            // valid discovery responses are handled in handlePacketInternal()
            if (_getCurrentMillis() - _lastByteReceived >= DISCOVERY_TIMEOUT) {
                if (_curReadIdx > 0 && !_lastCrcCorrect) {  // if bytes in buffer, but no/wrong CRC -> indicates collision
                    // traverse tree deeper / subdivide search range
                    if (curStep.level < 64) {
                        // push new instructions to stack
                        discoveryStack[discoveryStackIdx++] = {.baseAddr = addrMin, .level = (uint8_t)(curStep.level + 1), .upperHalf = true};
                        discoveryStack[discoveryStackIdx++] = {.baseAddr = addrMin, .level = (uint8_t)(curStep.level + 1), .upperHalf = false};
                    }
                    else {
                        // maximum tree depth reached, there should be no collision anymore, this indicates a problem
                        printf("Error: Maximum tree depth reached, there should be no collision anymore\n");
                    }
                }
                else {                                      // timeout without response
                    // probably no device in this address segment, remove segment from stack
                    discoveryStackIdx--;
                    // (handling for empty stack below)
                }

                discoveryState = DiscoveryIdle;
            }
            break;
        }
        case DiscoveryWaitingForSilenceACK: {
            // valid discovery responses are handled in handlePacketInternal()
            if (_getCurrentMillis() - _lastByteReceived >= DISCOVERY_TIMEOUT) {
                if (_curReadIdx > 0 && !_lastCrcCorrect) {  // if bytes in buffer, but no/wrong CRC -> indicates collision
                    // no collision should be indicated for discovery silence
                    printf("Error: Collision indicated for discovery silence, some logic is probably wrong\n");
                }
                else {
                    // timeout without response, probably wrong UID
                    printf("Warning: Discovery Silence timeout, wrong UID?\n");
                }
                discoveryState = DiscoveryIdle;
            }
            else if (_discoverySilenceCollision) {
                if (curStep.level < 64) {
                    // push new instructions to stack
                    discoveryStack[discoveryStackIdx++] = {.baseAddr = addrMin, .level = (uint8_t)(curStep.level + 1), .upperHalf = true};
                    discoveryStack[discoveryStackIdx++] = {.baseAddr = addrMin, .level = (uint8_t)(curStep.level + 1), .upperHalf = false};
                }
                discoveryState = DiscoveryIdle;
            }
            break;
        }
        case DiscoveryGotValidUID: {
            if (discoveredUidsNum >= _discoveredUidsSize) {
                // more UIDs found than space in buffer, return
                printf("Error: More UIDs found than space in buffer\n");
                return -2;
            }
            // search range again to make sure no devices are missed (don't pop stack)
            discoveryState = DiscoveryIdle;
            break;
        }
    }

    if (discoveryStackIdx == 0) {   // done
        return discoveredUidsNum;
    }
    else {
        return -1;
    }
}

size_t RBLB_Host::sendSimpleData(const uint8_t *payload, size_t size) {
    uint16_t packetSize = sizeof(simpleCommHeader_t) + size;
    uint8_t buf[packetSize] = {0};
    simpleCommHeader_t *header = (simpleCommHeader_t*)buf;

    header->cmd = RBLB::CMD::DataSimple;
    header->len = size;
    for (size_t i = 0; i < size; i++) {
        header->chkSum += payload[i];
        header->chkXor ^= payload[i];
    }
    memcpy(buf + sizeof(simpleCommHeader_t), payload, size);

    _sendBytes(buf, packetSize);
    return packetSize;
}

void RBLB_Host::setParameter(uint64_t uid, RBLB::ParamID paramId, uint32_t value) {
    RBLB::cmd_param_t param = {.paramId = paramId, .u32 = value};
    sendPacket(RBLB::CMD::SetParameters, uid, (uint8_t*)&param, sizeof(RBLB::cmd_param_t));
}

#endif