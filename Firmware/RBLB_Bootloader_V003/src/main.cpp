#include "ch32v003fun.h"
#include <stdio.h>
#include "rblb.h"
#include "crc.h"

#include "globals.h"
#include "uart.h"

// RBLB *rblb;


uint32_t count;

// You can override the interrupt vector this way:
void InterruptVector()         __attribute__((naked)) __attribute((section(".init")));
void InterruptVector()
{
	asm volatile( "\n\
	.align  2\n\
	.option   push;\n\
	.option   norvc;\n\
	j handle_reset\n\
	.option pop");
}

// reload_val: 12 bit
// prescaler: from 128 kHz LSI clock
static void iwdg_setup(uint16_t reload_val, uint8_t prescaler) {
	IWDG->CTLR = 0x5555;
	IWDG->PSCR = prescaler;

	IWDG->CTLR = 0x5555;
	IWDG->RLDR = reload_val & 0xfff;

	IWDG->CTLR = 0xCCCC;
}

static void iwdg_feed() {
	IWDG->CTLR = 0xAAAA;
}


int main()
{
	SystemInit();

	// RBLB rblbInstance(0, nullptr, nullptr, nullptr);
	// rblbInstance.handleByte('a');

	
	funGpioInitAll();

	funPinMode(PIN_LED1, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
	funPinMode(PIN_LED2, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
	funDigitalWrite(PIN_LED1, 1);	// turn off LEDs
	funDigitalWrite(PIN_LED2, 1);

	funPinMode(PIN_RS485_DE, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
	funPinMode(PIN_RS485_RE, GPIO_Speed_10MHz | GPIO_CNF_OUT_PP);
	funDigitalWrite(PIN_RS485_DE, 0);	// disable DE
	funDigitalWrite(PIN_RS485_RE, 0);	// enable RE (active low)

	// UART gets setup by fun framework, due to the define in funconfig.h

	// Activate watchdog, so we land back in the bootloader should something happen to the main application
	iwdg_setup(4095, IWDG_Prescaler_128);	// prsc=128 -> 1kHz WDG Timer => reload_val ~= ms

	// If RS485 bus is dominant for 0.5s at bootup, stay in bootloader regardless
	uint32_t start = SysTick->CNT;
	while (start - SysTick->CNT < (500 * DELAY_MS_TIME)) {
		if (funDigitalRead(PIN_RX)) {	// if RX pin goes high (RS485 idle) at any point, don't stay in bootloader
			// TODO: jump to user code
		}
	}

	while (true) {
		iwdg_feed();
		uartLoop();
	}

	

	



// ch32v003fun example code



	// From here, you can do whatever you'd like!
	// This code will live up at 0x1ffff000.

	// Enable GPIOD + C
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOC;

	// GPIO D0 Push-Pull, 10MHz Output
	GPIOD->CFGLR &= ~(0xf<<(4*0));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0);

	// GPIO D4 Push-Pull, 10MHz Output
	GPIOD->CFGLR &= ~(0xf<<(4*4));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*4);

	// GPIO C0 Push-Pull, 10MHz Output
	GPIOC->CFGLR &= ~(0xf<<(4*0));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0);


	static const uint32_t marker[] = { 0xaaaaaaaa };
	count = marker[0];

	int i;

	// Make a clear signature.
	for( i = 0; i < 10; i++ )
	{
		GPIOD->BSHR = 1 | (1<<4);                // Turn on GPIOD0 + D4
		GPIOC->BSHR = 1;                         // Turn on GPIOC0
		GPIOD->BSHR = (1<<16) | (1<<(16+4));     // Turn off GPIOD0 + D4
		GPIOC->BSHR = 1<<16;                     // Turn off GPIOC0
	}

	for( i = 0; i < 1; i++ )
	{
		GPIOD->BSHR = 1 | (1<<4);
		GPIOC->BSHR = 1;
		Delay_Ms( 250 );
		GPIOD->BSHR = (1<<16) | (1<<(16+4)); // Turn off GPIOD0 + D4
		GPIOC->BSHR = 1<<16;                     // Turn off GPIOC0
		Delay_Ms( 20 );
		count++;
	}

	// Exit bootloader after 5 blinks.

	// Note we have to do this if we ended up in the bootloader because
	// the main system booted us here.  If you don't care, you don't need
	// to turn OBTKEYR back off.
	FLASH->KEYR = FLASH_KEY1;
	FLASH->KEYR = FLASH_KEY2;
	FLASH->BOOT_MODEKEYR = FLASH_KEY1;
	FLASH->BOOT_MODEKEYR = FLASH_KEY2;
	FLASH->STATR = 0; // 1<<14 is zero, so, boot user code.
	FLASH->CTLR = CR_LOCK_Set;

	PFIC->SCTLR = 1<<31;
	while(1);
}