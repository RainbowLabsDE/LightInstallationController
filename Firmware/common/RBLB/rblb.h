// RainBow(Labs) Light Bus
#pragma once

#include <stdint.h>
#include <stddef.h>

class RBLB {
    public:

    enum CMD : uint8_t {
        Data = 1,           // data using previously configured values
        DataSimple,         // data with simple protocol (TODO)

        DiscoveryInit = 16, // bring all nodes into discoverable state
        DiscoveryBurst,     // all addressed nodes answer
        DiscoverySilence,   // silence a specific node

        SetParameters = 32,       
        GetParameters,
        GetStatus,          // Voltage / Temperature / uptime? / ???
        Reset,
        SaveParameters,     // Saves parameters to flash

        // Bootloader
        FlashStart = 0x70,   
        FlashPage,
        FlashDone,
    };

    static const uint8_t Response = 0x80;

    enum ParamID : uint8_t {
        NodeNum = 1,        // u16, node number (in WS2812 mode: starting LED number)
        GlobalBrightness,   // 0 - 255, unused
        GammaEnable,        // bool, unused
        Baudrate,           // u32, unnused

        // PWM specific
        NumChannels_PWM,    // 1 - 6
        BitsPerColor_PWM,   // 8 - 16
        BitsPerColor_Data,  // 8/16 

        // WS2812 specific
        BytesPerLED,        // 3-4, amount of color channels per LED
        NumLEDs,            // u16, number of LEDs in the string
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
        uint16_t len;           // length of data (excluding header)
        uint8_t chkSum;         // checksum, summed payload bytes
        uint8_t chkXor;         // checksum, xored payload bytes
        uint8_t data[];
    } simpleCommHeader_t;

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

    typedef union {
        struct {
            uint32_t address;
            uint8_t data[64];
        };
        uint16_t completeCrc;
    } cmd_flashPage_t;
    #pragma pack(pop)


    static const uint64_t ADDR_HOST = 0;
    static const uint64_t ADDR_BROADCAST = UINT64_MAX;
    static const unsigned PACKET_TIMEOUT = 2;  // ms, after that time a incomplete packet is discarded
    static const unsigned DISCOVERY_TIMEOUT = 2;  // ms (is actually 1-2ms because of timing. TODO: maybe switch to micros()?)

    RBLB(
        void (*txFunc)(const uint8_t *buf, size_t size),
        void (*packetCallback)(uidCommHeader_t *header, uint8_t *payload, RBLB* rblbInst),
        uint32_t (*getCurrentMillis)(),
        uint8_t *dataBuf = nullptr,
        const size_t dataBufSize = 0
        ) : _sendBytes(txFunc), _packetCallback(packetCallback), _getCurrentMillis(getCurrentMillis), _dataBuf(dataBuf), _dataBufSize(dataBufSize) { }

    void setUid(uint64_t ownUID) { _uid = ownUID; }

    // function to call when LED data has been sucessfully received
    void setDataCallback(void (*dataCallback)(uint8_t *data, size_t receivedBytes)) { _dataCallback = dataCallback; }
    // only return data from this area
    // TODO: maybe set bits/color, nodeID etc directly instead? (scope of this library?)
    void setDataWindow(uint16_t start, uint16_t length) { _dataStart = start; _dataLen = length; }

    void handleByte(uint8_t byte);
    size_t sendPacket(uint8_t cmd, uint64_t dstUid = 0, const uint8_t *payload = NULL, size_t payloadSize = 0);
    void idleLineReceived();
    void loop();                    // call repeatedly when RBLB_LOWOVERHEAD is set (used for protocol timeout)


    protected:
    virtual void handlePacketInternal(uidCommHeader_t *header, uint8_t *payload);

    void (*_sendBytes)(const uint8_t *buf, size_t size) = nullptr;
    void (*_packetCallback)(uidCommHeader_t *header, uint8_t *payload, RBLB* rblbInst) = nullptr;
    uint32_t (*_getCurrentMillis)() = nullptr;
    void (*_dataCallback)(uint8_t *data, size_t receivedBytes) = nullptr;

    // Node
    uint64_t _uid;
    bool _discovered = false;

    bool _idleLineReceived = false;
    uint8_t _packetBuf[32];     // buffer used for handling uid-based command packets
    uint16_t _curReadIdx = 0;
    uint32_t _lastByteReceived = 0;
    bool _lastCrcCorrect = false;

    // buffer for storing received LED data frame
    uint8_t* _dataBuf;
    const size_t _dataBufSize;
    uint16_t _dataStart = 0, _dataLen = 0;
    uint8_t _dataChkSum = 0, _dataChkXor = 0;
    
};
