#include "rblb.h"
#include "../CRC/crc.h"

#include <stdio.h>
#include <string.h>

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
    uint64_t addrMin = curStep.baseAddr + curStep.upperHalf ? levelMask : 0;
    uint64_t addrMax = addrMin + (levelMask - 1);


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