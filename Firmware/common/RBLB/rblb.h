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
        uint8_t cmd;
        uint64_t address;
        uint16_t len;           // length of data excluding header and checksum
        uint8_t data[];
        // uint16_t checksum;   // checksum is at end of data array
    } uidCommHeader_t;

    typedef struct {
        uint8_t cmd;
        uint8_t data[];
    } basicCommHeader_t;

    typedef struct {
        uint8_t paramId;
        union {
            uint8_t u8;
            uint16_t u16;
            uint32_t u32;
        };
    } cmd_param_t;

    typedef union {
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

    RBLB(uint64_t ownUID,
        void (*txFunc)(const uint8_t *buf, size_t size),
        void (*packetCallback)(uidCommHeader_t *header, uint8_t *payload),
        uint32_t (*getCurrentMillis)()
        );

    void handleByte(uint8_t byte);
    size_t sendPacket(uint8_t cmd, uint64_t dstUid = 0, const uint8_t *payload = NULL, size_t payloadSize = 0);
    void idleLineReceived();


    private:
    void handlePacketInternal(uidCommHeader_t *header, uint8_t *payload);

    void (*_sendBytes)(const uint8_t *buf, size_t size);
    void (*_packetCallback)(uidCommHeader_t *header, uint8_t *payload);
    uint32_t (*_getCurrentMillis)();

    uint64_t _uid;
    bool _discovered = false;
    bool _idleLineReceived = false;
    uint8_t _packetBuf[32];     // buffer used for handling uid-based command packets
    uint16_t _curReadIdx = 0;
    uint32_t _lastByteReceived = 0;
};