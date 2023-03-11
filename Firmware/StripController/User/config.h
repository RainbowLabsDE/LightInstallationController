#include <stdint.h>

typedef struct {
    uint16_t preamble;
    uint8_t configVersion;
    uint16_t addressOffset;
    uint8_t numOutputs;
    uint8_t bitDepthPWM;
    uint8_t bitDepthData;
} config_t;

extern config_t _config;