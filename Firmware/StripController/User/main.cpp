#include "debug.h"
#include "uart.h"
#include "util.h"
#include "config.h"
#include "../../common/RBLB/rblb.h"

vu8 val;
config_t _config;

// R  - T1CH1 - PC6
// G  - T1CH2 - PC7
// B  - T1CH3 - PC0
// W  - T1CH4 - PD3
// WW - T2CH2 - PD4
// TX - PD5
// RX - PD6

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

void rblbPacketCallback(RBLB::uidCommHeader_t *header, uint8_t *payload) {

}

void rs485Write(const uint8_t *buf, size_t size) {
    // TODO: how to handle blocking?
    // set DE
    uart1.sendBytes(buf, size);
    // clear DE
}


int main(void) {
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    SysTickInit();
    uart1.init();
    TIM1_PWMOut_Init((1 << 14) - 2, 0, 1);

    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("Chip ID: %08lX %08lX\n", (uint32_t)(getUID() >> 32), getUID());
    
    printf("RevID: %04X, DevID: %04x\n", DBGMCU_GetREVID(), DBGMCU_GetDEVID());

    RBLB rblb(getUID(), rs485Write, rblbPacketCallback, millis);

    while (1) {
        while (uart1.available()) {
            uint8_t c = uart1.read();
            printf("%c", c);
            printf("%8ld %8ld\n", millis(), micros());
            rblb.handleByte(c);
        }
    }
}

extern "C" {
// needed for C++ compatibility, apparently
void _fini() {}
void _init() {}
}
