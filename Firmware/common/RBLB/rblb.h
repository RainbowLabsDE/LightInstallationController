// RainBow(Labs) Light Bus

#include <stdint.h>

class RBLB {
    public:

    enum CMD : uint8_t {
        Data,               // data using previously configured values
        DataSimple,         // data with simple protocol
        DiscoveryInit = 16, // bring all nodes into discoverable state
        DiscoveryBurst,     // all addressed nodes answer
        DiscoverySilence,   // silence a specific node
        SetParameters,       
        GetParameters,
    };

    enum Params : uint8_t {
        BitsPerColor_PWM,   // 8 - 16
        BitsPerColor_Data,  // 8/16 
        NumChannels,        // 3 - 6
        NodeNum,            // 16 bit node number
        GlobalBrightness,
        GammaEnable,
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
    #pragma pack(pop)


    RBLB(uint64_t ownUID);

    void handleByte(uint8_t byte);
    void idleLineReceived();


    private:
    uint64_t _uid;
    bool _discovered = false;
    bool _idleLineReceived = false;
    uint8_t _packetBuf[32];     // buffer used for handling uid-based command packets
    uint8_t _curReadIdx = 0;
};