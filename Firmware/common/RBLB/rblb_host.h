#pragma once
#include "rblb.h"

class RBLB_Host : public RBLB {
    public:
    RBLB_Host(uint64_t ownUID,
        void (*txFunc)(const uint8_t *buf, size_t size),
        void (*packetCallback)(uidCommHeader_t *header, uint8_t *payload),
        uint32_t (*getCurrentMillis)()
        ) : RBLB(ownUID, txFunc, packetCallback, getCurrentMillis) {};

    // Needs to be called multiple times until done (nonblocking, because incoming byte handling)
    // returns -1 when not done, else number of discovered nodes
    void discoveryInit(uint64_t *discoveredUids, size_t size);
    int discoverNext();

    protected:
    // void handlePacketInternal(uidCommHeader_t *header, uint8_t *payload);
    // Host (TODO: leave out from node instance somehow. Inheritance / Templating?)
    // bool _waitingForDiscoveryResponse = false;
    // bool _someByteReceived = false;

    bool _discoverySilenceCollision = false;

    #pragma pack(push, 1)
    typedef struct {
        uint64_t baseAddr;
        uint8_t level;
        bool upperHalf;
    } discovery_step_t;
    #pragma pack(pop)

    discovery_step_t *discoveryStack = NULL;
    size_t discoveryStackSize = 256;
    size_t discoveryStackIdx = 0;   // points to next empty slot in stack

    // uint64_t discoveredValidUid = 0;
};