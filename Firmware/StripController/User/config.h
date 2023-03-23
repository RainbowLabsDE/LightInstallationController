#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t preamble;
    uint8_t configVersion;

    uint16_t addressOffset;
    uint8_t numOutputs;
    uint8_t bitDepthPWM;
    uint8_t bitDepthData;
    
    uint16_t crc;
} config_t;
#pragma pack(pop)

extern config_t _config;