/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "main.h"
#include "config.h"
#include <avr/boot.h>

#define FQ 100

volatile uint8_t gTimeout;
static volatile uint8_t lTimerTicks = 0;
volatile uint8_t gLedOnTime = 0;

static void (*nullVector)(void) __attribute__((__noreturn__)) = 0x0000;
void main() __attribute__ ((noreturn));

void leaveBootloader()
{
    cli();
    boot_rww_enable_safe();
	
	// check if first page is clear, if so - this function will fail
	if (pgm_read_byte_near(0x01) == 0xFF)
	{
		gLedOnTime = 100;
		return;
	}

	TIMSK1 = 0;
	TCCR1A = 0;
	TCCR1B = 0;
	
	TIMSK0 = 0;
	TCCR0A = 0;
	TCCR0B = 0;

	TIMSK2 = 0;
	TCCR2A = 0;
	TCCR2B = 0;
	
	SPSR = 0;
	SPCR = 0;

	PCICR 	= 0;
	PCMSK0 	= 0;

	DDRB 	= 0;
	PORTB 	= 0;
	
    MCUCR = (1 << IVCE);     /* enable change of interrupt vectors */
    MCUCR = (0 << IVSEL);    /* move interrupts to application flash section */

	wdt_enable(WDTO_2S);	// if something went wrong watchdog should reset mcu
	
    nullVector();
}
 
void timer1_ctc_init(void)
{    
	TCCR1A    |= (0<<WGM11)|(0<<WGM10);
	TCCR1B    |= (0<<WGM13)|(1<<WGM12); // CTC mode
	TIMSK1     = 1<<OCIE1A;
	OCR1A      = ((F_CPU / 8) / FQ) - 1;
	//OCR1A      = 1000; // 100Hz
	//OCR1B      = 50000; // 50Hz
	TCCR1B    |= (0<<CS12)|(1<<CS11)|(0<<CS10);

	TIMSK1     = (1<<OCIE1A);// | (1<<OCIE1B);	
}

ISR(TIMER1_COMPA_vect)
{
	lTimerTicks++;

	if ((lTimerTicks % (FQ / 4)) == 0)
	{
		gLedOnTime = 5;
	}

	if (lTimerTicks == FQ)
	{
		lTimerTicks = 0;
		gTimeout--;
		
		if (gTimeout == 0)
		{
			leaveBootloader();
		}
	}
	
	if (gLedOnTime)
	{
		PORTB &= ~(1 << PB1);
		gLedOnTime--;
	}
	
	if (gLedOnTime == 0)
	{
		PORTB |= (1 << PB1);
	}
}

ISR(PCINT0_vect) 
{
	if ((PINB & (1 << PB0)) == 0)
	{
		while(ethernet_handle_packet());
	}
}

void main(void)
{
	gTimeout = 3;
	
	//ispConnect();
	//ispDisconnect();
	//ispEnterProgrammingMode();
	//ispWriteFlash(1, 1, 0);
	//ispFlushPage(0, 0);
	
	MCUCR = (1<<IVCE);
	MCUCR = (1<<IVSEL);

	TIMSK0 = 0;
	TCCR0A = 0;
	TCCR0B = 0;

	TIMSK2 = 0;
	TCCR2A = 0;
	TCCR2B = 0;	
	
	wdt_disable();
	config_read();
		
	//DDRD &= (0x01 << 5); // fan
	//PORTD &= (0x01 << 5); // fan

	//DDRC &= (1 << PC3) | (1 << PC1);
	
	DDRB &= ~(0x01 << 0);	// enc interrupt
	PORTB &= ~(0x01 << 0);

	DDRB |= 0x01 << 1; // led
	PORTB |= (0x01 << 1);
	
	PCICR |= _BV(PCIE0);
	PCMSK0 |= _BV(PCINT0);
			
	/* init arch */
	spi_init();	
	timer1_ctc_init();

	/* init dev */
	enc28j60_init((uint8_t*)&gConfig.netMac, NULL); 
	ethernet_init((const ethernet_address*)&gConfig.netMac);
	
	ip_init(
		(const ip_address*)&gConfig.netIp, 
		(const ip_address*)&gConfig.netMask, 
		(const ip_address*)&gConfig.netGateway);

	arp_init();

#if defined(SYS_ENABLE_AS110x)
	as110x_init();
	as110x_writeAll(AS110X_INTENSITY_CTRL, 0x03);
	as110x_writeAll(AS110X_DECODE_MODE, 0xFF);
	
	for (int8_t i = 1; i <= 8; ++i)
	{
		as110x_writeAll(i, 0x0F);
	}		
	
	for (int8_t i = 1; i <= 8; ++i)
	{
		if (i <= 6)
		{
			as110x_writeOne(0, i, 0x0A);
		}
	}		
#endif
	
	sei();
	for(;;)
	{
	}
}