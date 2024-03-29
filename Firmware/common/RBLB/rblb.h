// RainBow(Labs) Light Bus
#pragma once

#include <stdint.h>
#include <stddef.h>

class RBLB {
    public:

    enum CMD : uint8_t {
        Data,               // data using previously configured values
        DataSimple,         // data with simple protocol (TODO)

        DiscoveryInit = 16, // bring all nodes into discoverable state
        DiscoveryBurst,     // all addressed nodes answer
        DiscoverySilence,   // silence a specific node

        SetParameters = 32,       
        GetParameters,
        GetStatus,          // Voltage / Temperature / uptime? / ???
    };

    static const uint8_t Response = 0x80;

    enum ParamID : uint8_t {
        NodeNum,            // 16 bit node number
        NumChannels,        // 3 - 6
        BitsPerColor_PWM,   // 8 - 16
        BitsPerColor_Data,  // 8/16 
        GlobalBrightness,   // 0 - 255
        GammaEnable,        // bool
        Baudrate,
    };

    #pragma pack(push, 1)
    typedef struct {
        uint8_t cmd;            // highest bit set indicates response from node
        uint64_t address;       // in single-master mode, it contains both destination and source address, depending on direction
        uint16_t len;           // length of data (excluding header and checksum)
        uint8_t data[];
        // uint16_t checksum;   // checksum is at end of data array
    } uidCommHeader_t;

    typedef struct {
        uint8_t cmd;
        uint8_t data[];
    } basicCommHeader_t;

    // Command Payloads

    typedef struct {        // DiscoveryBurst (request)
        uint64_t searchAddress_min;
        uint64_t searchAddress_max; // inclusive
    } cmd_discovery_burst_t;

    typedef struct {        // GetParameters (request)
        uint8_t paramId;
        union {
            uint8_t u8;
            uint16_t u16;
            uint32_t u32;
        };
    } cmd_param_t;

    typedef union {         // GetStatus (response)
        struct {
            uint16_t vBusAdc;
            uint16_t tempAdc;
            uint32_t uptimeMs;
        } cmd_status_s;
        uint8_t raw[sizeof(cmd_status_s)];
    } cmd_status_t;
    #pragma pack(pop)


    static const uint64_t ADDR_HOST = 0;
    static const uint64_t ADDR_BROADCAST = UINT64_MAX;
    static const unsigned PACKET_TIMEOUT = 5;  // ms, after that time a incomplete packet is discarded
    static const unsigned DISCOVERY_TIMEOUT = 2;  // ms (is actually 1-2ms because of timing. TODO: maybe switch to micros()?)

    RBLB(uint64_t ownUID,
        void (*txFunc)(const uint8_t *buf, size_t size),
        void (*packetCallback)(uidCommHeader_t *header, uint8_t *payload),
        uint32_t (*getCurrentMillis)()
        );

    void handleByte(uint8_t byte);
    size_t sendPacket(uint8_t cmd, uint64_t dstUid = 0, const uint8_t *payload = NULL, size_t payloadSize = 0);
    void idleLineReceived();


    protected:
    void handlePacketInternal(uidCommHeader_t *header, uint8_t *payload);

    void (*_sendBytes)(const uint8_t *buf, size_t size);
    void (*_packetCallback)(uidCommHeader_t *header, uint8_t *payload);
    uint32_t (*_getCurrentMillis)();

    // Node
    uint64_t _uid;
    bool _discovered = false;

    bool _idleLineReceived = false;
    uint8_t _packetBuf[32];     // buffer used for handling uid-based command packets
    uint16_t _curReadIdx = 0;
    uint32_t _lastByteReceived = 0;

    // Host (TODO: leave out from node instance somehow. Inheritance / Templating?)
    bool _lastCrcCorrect = false;
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
};

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