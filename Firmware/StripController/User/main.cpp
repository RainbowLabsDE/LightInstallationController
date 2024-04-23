#include "debug.h"
#include "uart.h"
#include "util.h"
#include "config.h"
#include "../../common/RBLB/rblb.h"
#include "adc.h"

vu8 val;
Config config;

// R  - T1CH3 - PC0
// G  - T1CH1 - PC6 | WS2812 DOUT
// B  - T1CH2 - PC7
// W  - T1CH4 - PD3
// WW - T2CH2 - PD4
// TX - PD5
// RX - PD6
// LED1 - PC1
// LED2 - PC2
// DE - PC4
// RE - PC5 (active low)
// VBUS - PC3 - A?? (wrong pin?) -> PA2 (A0)
// TEMP - PD2 - A3

#define PIN_LED1        GPIOC, GPIO_Pin_1
#define PIN_LED2        GPIOC, GPIO_Pin_2
#define PIN_RS485_DE    GPIOC, GPIO_Pin_4
#define PIN_RS485_RE    GPIOC, GPIO_Pin_5

#define NO_DATA_TIMEOUT 2000

#define LED_DATA_BUF_SIZE   600     // also used for the internal RBLB buffer size (for double buffering)
uint8_t ledData[LED_DATA_BUF_SIZE] = {0};

uint32_t lastDataReceived = 0;

#if VARIANT_PWM

/*********************************************************************
 * @fn      TIM1_OutCompare_Init
 *
 * @brief   Initializes TIM1 output compare.
 *
 * @param   arr - the period value.
 *          psc - the prescaler value.
 *          ccp - the pulse value.
 *
 * @return  none
 */
void TIM1_PWMOut_Init(u16 arr, u16 psc, u16 ccp) {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_PartialRemap1_TIM1, ENABLE); // CH1-4: PC6, PC7, PC0, PD3

    GPIO_InitTypeDef GPIO_InitStructure = {0};
    TIM_OCInitTypeDef TIM_OCInitStructure = {0};
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_TIM1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    TIM_TimeBaseInitStructure.TIM_Period = arr;
    TIM_TimeBaseInitStructure.TIM_Prescaler = psc;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

#if (PWM_MODE == PWM_MODE1)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;

#elif (PWM_MODE == PWM_MODE2)
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;

#endif

    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = ccp;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);
    TIM_OC4Init(TIM1, &TIM_OCInitStructure);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Disable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

void setPwmOutputs(uint16_t o1, uint16_t o2, uint16_t o3, uint16_t o4 = 0, uint16_t o5 = 0, uint16_t o6 = 0) {
    TIM_SetCompare3(TIM1, o1);
    TIM_SetCompare1(TIM1, o2);
    TIM_SetCompare2(TIM1, o3);
    TIM_SetCompare4(TIM1, o4);
    // o5
}

void setPwmOutput(int chan, uint16_t outputVal) {
    switch (chan) {
        case 0: TIM_SetCompare3(TIM1, outputVal);   break;  // R
        case 1: TIM_SetCompare1(TIM1, outputVal);   break;  // G
        case 2: TIM_SetCompare2(TIM1, outputVal);   break;  // B
        case 3: TIM_SetCompare3(TIM1, outputVal);   break;  // W
    }
}
#endif

#if VARIANT_WS2812

#define WS2812DMA_IMPLEMENTATION
#define WSRGB

extern "C" { void DMA1_Channel3_IRQHandler( void ) __attribute__((interrupt("WCH-Interrupt-fast"))); }
#include "ws2812b_dma_spi_led_driver.h"

uint32_t WS2812BLEDCallback( int ledno ) {
    // return 0xFF << (8 * ((millis() / 1000) % 3));
    // return 1 << (ledno + (millis() >> 10) % 24);
    // return 0x110000;
    if (ledno < (sizeof(ledData) / 3)) {
        uint32_t data = 0;
        memcpy(&data, ledData + (ledno * 3), 3);
        return data;
    }
    return 0;
}
#endif

void gpioInit() {
    GPIO_InitTypeDef gpioInitStruct = {
        .GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_4 | GPIO_Pin_5,
        .GPIO_Speed = GPIO_Speed_50MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP,
    };
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    GPIO_Init(GPIOC, &gpioInitStruct);
    
    GPIO_WriteBit(PIN_LED1, Bit_SET);       // turn off LED
    GPIO_WriteBit(PIN_LED2, Bit_SET);       // turn off LED
    GPIO_WriteBit(PIN_RS485_DE, Bit_RESET); // disable send
    GPIO_WriteBit(PIN_RS485_RE, Bit_RESET); // enable receive (active low)
}

void updateDataWindow(RBLB* rblbInst) {
    #if VARIANT_PWM
        int dataWindowLen = ((config._config.bitDepthData + 7) / 8) * config._config.numOutputs;
        int dataWindowOffs = config._config.nodeNum * dataWindowLen;
        rblbInst->setDataWindow(dataWindowOffs, dataWindowLen);
    #elif VARIANT_WS2812
        int dataWindowLen =  config._config.numLEDs * config._config.bytesPerLED;
        int dataWindowOffs = config._config.nodeNum * config._config.bytesPerLED;
        rblbInst->setDataWindow(dataWindowOffs, dataWindowLen);
    #endif
}

void rblbPacketCallback(RBLB::uidCommHeader_t *header, uint8_t *payload, RBLB* rblbInst) {
    switch (header->cmd) {
        case RBLB::CMD::SetParameters: {
            RBLB::cmd_param_t *paramCmd = (RBLB::cmd_param_t*)payload;
            
            switch (paramCmd->paramId) {
                case RBLB::ParamID::NodeNum:
                    config._config.nodeNum = paramCmd->u16;
                    break;
                case RBLB::ParamID::NumChannels_PWM:
                    config._config.numOutputs = paramCmd->u8;
                    // TODO: reconfigure PWM outputs
                    break;
                case RBLB::ParamID::BitsPerColor_PWM:
                    config._config.bitDepthPWM = paramCmd->u8;
                    break;
                case RBLB::ParamID::BitsPerColor_Data:
                    config._config.bitDepthData = paramCmd->u8;
                    break;
                case RBLB::ParamID::NumLEDs:
                    config._config.numLEDs = paramCmd->u16;
                    break;
                case RBLB::ParamID::BytesPerLED:
                    config._config.bytesPerLED = paramCmd->u8;
                    break;
            }
           
            updateDataWindow(rblbInst);

            break;
        }
        case RBLB::CMD::GetStatus: {
            RBLB::cmd_status_t pkt = { .cmd_status_s = {
                .vBusAdc = adcSampleBuf[0],
                .tempAdc = adcSampleBuf[1],
                .uptimeMs = millis(),  
            }};
            rblbInst->sendPacket(header->cmd, getUID(), pkt.raw, sizeof(pkt));
            break;
        }
        case RBLB::CMD::Reset:
            NVIC_SystemReset();
            break;
    }
}

void rblbDataCallback(uint8_t *data, size_t receivedBytes) {
    lastDataReceived = millis();
    #if VARIANT_PWM
        int bitsData = config._config.bitDepthData;
        int bitsPWM = config._config.bitDepthPWM;
        int dataBytesPerChan = 1 + (bitsData > 8);  // currently only handles full 8/16-bit data per channel
        for (int channel = 0; channel < config._config.numOutputs; channel++) {
            size_t dataOffset = channel * dataBytesPerChan;

            if (dataOffset >= receivedBytes) {
                return;
            }

            uint16_t value = data[dataOffset];
            if (dataBytesPerChan == 2) {
                value |= data[dataOffset + 1] << 8;     // channel data is assumed little-endian
            }
            
            // simply shift the given data bits into the required PWM bits, if mismatched
            if (bitsData < bitsPWM) {
                value <<= (bitsPWM - bitsData);
            }
            else if (bitsData > bitsPWM) {
                value >>= (bitsData - bitsPWM);
            }

            setPwmOutput(channel, value);
        }
    #endif 

    #if VARIANT_WS2812
        if (receivedBytes < sizeof(ledData)) {
            memcpy(ledData, data, receivedBytes);
            if (!WS2812BLEDInUse) {
                WS2812BDMAStart(receivedBytes / config._config.bytesPerLED);
            }
        }
    #endif
}

inline void rs485_de() {
    GPIO_WriteBit(PIN_RS485_DE, Bit_SET);
    GPIO_WriteBit(PIN_RS485_RE, Bit_SET);
}
inline void rs485_re() {
    GPIO_WriteBit(PIN_RS485_DE, Bit_RESET);
    GPIO_WriteBit(PIN_RS485_RE, Bit_RESET);
}

void rs485Write(const uint8_t *buf, size_t size) {
    // TODO: how to handle blocking?
    // set DE
    rs485_de();
    uart1.sendBytes(buf, size);
    // clear DE
    rs485_re();
}

// TODO: somehow move buffer definition into class itself, maybe via templating? (but still show in memory usage)
uint8_t internalLedDataBuf[LED_DATA_BUF_SIZE] = {0};
RBLB rblb(rs485Write, rblbPacketCallback, millis, internalLedDataBuf, LED_DATA_BUF_SIZE);


int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SysTickInit();
    gpioInit();
    uart1.init();
    // uart1.sendBytes((uint8_t*)"Hello World!", 13);

    printfd("SystemClk:%d\r\n", SystemCoreClock);
    printfd("Chip ID: %08lX %08lX\n", (uint32_t)(getUID() >> 32), getUID());
    
    printfd("RevID: %04X, DevID: %04x\n", DBGMCU_GetREVID(), DBGMCU_GetDEVID());
    #if VARIANT_PWM
        TIM1_PWMOut_Init((1 << 14) - 2, 0, 1);
        setPwmOutputs((1 << 14)/100, (1 << 14)/2, (1 << 14)/1000, 1);
    #endif
    #if VARIANT_WS2812
        WS2812BDMAInit();
        WS2812BDMAStart(LED_DATA_BUF_SIZE / 3); // set to black
    #endif
    adcInit();
    config.load();


    rblb.setUid(getUID());
    rblb.setDataCallback(rblbDataCallback);

    uint32_t lastPrint = 0;
    uint8_t serBuf[32];

    while (1) {
        // GPIOC->BSHR = GPIO_Pin_1;   // profiling
        // while (uart1.available()) {
        //     uint8_t c = uart1.read();
        //     // printfd("%c", c);
        //     // printfd("%8ld %8ld\n", millis(), micros());
        //     // if (toIgnore) {
        //     //     toIgnore--;
        //     //     continue;
        //     // }
        //     rblb.handleByte(c);
        // }
        // GPIOC->BCR = GPIO_Pin_1;

        // GPIOC->BSHR = GPIO_Pin_1;   // profiling
        size_t bytesRead = uart1.readBytes(serBuf, sizeof(serBuf)); // takes ~2us when not doing anything. When copying data, up to 10us was seen
        // GPIOC->BCR = GPIO_Pin_1;
        for (size_t i = 0; i < bytesRead; i++) {
            GPIOC->BSHR = GPIO_Pin_1;   // profiling
            rblb.handleByte(serBuf[i]);     // takes 3.2us for data bytes (and ~ 2.5us for header bytes), TODO: optimize
            GPIOC->BCR = GPIO_Pin_1;
        }

        if (millis() - lastDataReceived >= NO_DATA_TIMEOUT) {
            lastDataReceived = millis(); // don't repeat too often
            #if VARIANT_PWM
                setPwmOutputs(0, 0, 0, 0, 0, 0);
            #endif
            #if VARIANT_WS2812
                memset(ledData, 0, sizeof(ledData));
                WS2812BDMAStart(LED_DATA_BUF_SIZE / 3); // set to black
            #endif
        }

        // GPIO_WriteBit(PIN_LED1, Bit_SET);
        // delay(100);
        // GPIO_WriteBit(PIN_LED1, Bit_RESET);
        // delay(100);

        // if (millis() - lastPrint > 100) {
        //     lastPrint = millis();
        //     printfd("ADC: %4d %4d\n", adcSampleBuf[0], adcSampleBuf[1]);
        // }
    }
}

extern "C" {
// needed for C++ compatibility, apparently
void _fini() {}
void _init() {}
}
