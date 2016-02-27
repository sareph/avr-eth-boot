#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "../arch/spi.h"
#include <math.h>
#include "as110x.h"
#include "../config/config.h"
#include <util/delay.h>

#ifndef CONFIG_AS110X_SCAN_LIMIT
#define CONFIG_AS110X_SCAN_LIMIT 0x08
#endif

#ifndef SYS_AS110X_DISPLAYS
#define SYS_AS110X_DISPLAYS 1
#endif

void as110x_writeAll(uint8_t reg, uint8_t val)
{
	uint8_t i;
	AS110X_CS_ON();
	for (i = 0; i < SYS_AS110X_DISPLAYS; ++i)
	{
		spi_fast_shift(reg);
		spi_fast_shift(val);
	}
	AS110X_CS_OFF();	
}

void as110x_writeOne(int8_t disp, uint8_t reg, uint8_t value)
{
	uint8_t i;
	
	AS110X_CS_ON();
	for (i = disp + 1; i < SYS_AS110X_DISPLAYS; ++i)
	{
		spi_fast_shift(AS110X_NOOP);
		spi_fast_shift(0);
	}

	spi_fast_shift(reg);
	spi_fast_shift(value);
	
	for (i = 0; i < disp; ++i)
	{
		spi_fast_shift(AS110X_NOOP);
		spi_fast_shift(0);
	}
	
	AS110X_CS_OFF();
}

void as110x_init()
{
	AS110X_CS_DDR |= (1 << AS110X_CS_PIN);	
	AS110X_CS_OFF();
	
	as110x_writeAll(AS110X_TEST, 0x00);
	as110x_writeAll(AS110X_SHUTDOWN, 0x01);
	as110x_writeAll(AS110X_SCAN_LIMIT, CONFIG_AS110X_SCAN_LIMIT - 1);
}

void as110x_test(uint8_t bOn)
{
	as110x_writeAll(AS110X_TEST, bOn);
}

void as110x_set(uint8_t dig, uint8_t value, int8_t disp)
{
	if (disp != -1)
	{
		as110x_writeOne(disp, dig + 1, value);
	}
	else
	{
		as110x_writeAll(dig + 1, value);
	}
}