#include <stdint.h>
#include <string.h>
#include "../../common/CRC/crc.h"

class Config {
    public:


    #pragma pack(push, 1)
    typedef struct {
        uint8_t preamble;
        uint8_t configVersion;

        uint16_t addressOffset;
        uint16_t numOutputs : 3;    // <= 7
        uint16_t bitDepthPWM : 5;   // <= 31
        uint16_t bitDepthData : 5;  // <= 31
        
        uint16_t crc;
    } config_t;
    #pragma pack(pop)

    // writes config to flash
    void save() {
        config_t storedCfg = get();
        // check if config changed
        if (memcmp(&storedCfg, &_config, sizeof(config_t)) != 0) {
            _config.crc = crc16((uint8_t*)&_config, sizeof(config_t) - sizeof(config_t::crc));  // calculate CRC
            int status = flashOBWrite((uint8_t*)&_config, sizeof(config_t));                    // write to flash
            if (status < 0) {
                printfd("[CFG] Error during flash write: %d\n", status);
            }
        }
    }

    // loads config from flash
    void load() {
        config_t storedCfg = get();
        if (storedCfg.preamble != 0 && storedCfg.configVersion == defaultData.configVersion) {
            memcpy(&_config, &storedCfg, sizeof(config_t));
        }
        else {  // data corrupt / empty, load defaults
            printfd("[CFG] Config corrupt or version mismatch. Loading defaults!\n");
            memcpy(&_config, &defaultData, sizeof(config_t));
            save();
        }
    }

    // returns config from flash
    config_t get() {
        uint8_t tmp[sizeof(config_t)] = {0};
        int status = flashOBRead(tmp, sizeof(tmp));
        if (status >= 0) {  // read successful
            uint16_t crc = crc16(tmp, sizeof(tmp) - sizeof(config_t::crc));
            config_t *tmpCfg = (config_t*)tmp;
            if (crc == tmpCfg->crc) {
                return *tmpCfg;
            }
        }
        config_t empty = {0};
        return empty;
    }


    config_t _config;
    private:

    static constexpr config_t defaultData = {
        .preamble = 0x42,
        .configVersion = 0x01,

        .addressOffset = 0,
        .numOutputs = 3,
        .bitDepthPWM = 14,
        .bitDepthData = 8,
        
    };


};

extern Config config;