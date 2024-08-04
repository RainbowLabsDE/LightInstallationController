#pragma once
#include "rblb.h"

class RBLB_Host : public RBLB {
    public:
    RBLB_Host(uint64_t ownUID,
        void (*txFunc)(const uint8_t *buf, size_t size),
        void (*packetCallback)(uidCommHeader_t *header, uint8_t *payload, RBLB* rblbInst),
        uint32_t (*getCurrentMillis)()
        ) : RBLB(txFunc, packetCallback, getCurrentMillis) {
            setUid(ownUID);
        };

    // Needs to be called multiple times until done (nonblocking, because incoming byte handling)
    // returns -1 when not done, else number of discovered nodes
    void discoveryInit(uint64_t *discoveredUids, size_t size);
    int discoverNext();

    size_t sendSimpleData(const uint8_t *payload, size_t size);

    void setParameter(uint64_t uid, RBLB::ParamID paramId, uint32_t value);

    protected:
    void handlePacketInternal(uidCommHeader_t *header, uint8_t *payload) override;
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
    static const size_t discoveryStackSize = 256;
    size_t discoveryStackIdx = 0;   // points to next empty slot in stack

    typedef enum DiscoveryState : uint8_t {
        DiscoveryIdle,
        DiscoveryWaitingForResponse,
        DiscoveryWaitingForSilenceACK,
        DiscoveryGotValidUID,
    } discoveryState_t;

    discoveryState_t discoveryState = DiscoveryIdle;

    uint64_t *_discoveredUids = NULL;
    size_t _discoveredUidsSize = 0;
    size_t discoveredUidsNum = 0;

    // uint64_t discoveredValidUid = 0;
};