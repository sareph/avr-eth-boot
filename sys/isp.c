/*
 * isp.c - part of USBasp
 *
 * Autor..........: Thomas Fischl <tfischl@gmx.de>
 * Description....: Provides functions for communication/programming
 *                  over ISP interface
 * Licence........: GNU GPL v2 (see Readme.txt)
 * Creation Date..: 2005-02-23
 * Last change....: 2010-01-19
 */

#include <avr/io.h>
#include "isp.h"
#include "clock.h"
#include "usbasp.h"
#include <util/delay.h>
#include "../arch/spi.h"

#define spiHWdisable() SPCR = 0
#define ispDelay() _delay_us(1)

uchar sck_spcr;
uchar sck_spsr;
uchar isp_hiaddr;

void spiHWenable() {
	SPCR = sck_spcr;
	SPSR = sck_spsr;
}

void ispSetSCKOption(uchar option) {

	if (option == USBASP_ISP_SCK_AUTO)
		option = USBASP_ISP_SCK_375;

	if (option >= USBASP_ISP_SCK_93_75) {
		sck_spsr = 0;
		//sck_sw_delay = 1;	/* force RST#/SCK pulse for 320us */

		switch (option) {

		case USBASP_ISP_SCK_1500:
			/* enable SPI, master, 1.5MHz, XTAL/8 */
			sck_spcr = (1 << SPE) | (1 << MSTR) | (1 << SPR0);
			sck_spsr = (1 << SPI2X);
		case USBASP_ISP_SCK_375:
		default:
			/* enable SPI, master, 375kHz, XTAL/32 (default) */
			sck_spcr = (1 << SPE) | (1 << MSTR) | (1 << SPR1);
			sck_spsr = (1 << SPI2X);
			break;
		}

	}
}

void ispConnect() {

	/* all ISP pins are inputs before */
	/* now set output pins */
	ISP_DDR |= (1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI);

	/* reset device */
	ISP_OUT &= ~(1 << ISP_RST); /* RST low */
	ISP_OUT &= ~(1 << ISP_SCK); /* SCK low */

	/* positive reset pulse > 2 SCK (target) */
	ispDelay();
	ISP_OUT |= (1 << ISP_RST); /* RST high */
	ispDelay();
	ISP_OUT &= ~(1 << ISP_RST); /* RST low */

	spiHWenable();
	
	/* Initial extended address value */
	isp_hiaddr = 0;
}

void ispDisconnect() {

	/* set all ISP pins inputs */
	ISP_DDR &= ~((1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI));
	/* switch pullups off */
	ISP_OUT &= ~((1 << ISP_RST) | (1 << ISP_SCK) | (1 << ISP_MOSI));

	/* disable hardware SPI */
	spiHWdisable();
}

uchar ispEnterProgrammingMode() {
	uchar check;
	uchar count = 32;

	while (count--) {
		spi_fast_shift(0xAC);
		spi_fast_shift(0x53);
		check = spi_fast_shift(0);
		spi_fast_shift(0);

		if (check == 0x53) {
			return 0;
		}

		spiHWdisable();

		/* pulse RST */
		ispDelay();
		ISP_OUT |= (1 << ISP_RST); /* RST high */
		ispDelay();
		ISP_OUT &= ~(1 << ISP_RST); /* RST low */
		ispDelay();

		spiHWenable();
	}

	return 1; /* error: device dosn't answer */
}

static void ispUpdateExtended(unsigned long address)
{
	uchar curr_hiaddr;

	curr_hiaddr = (address >> 17);

	/* check if extended address byte is changed */
	if(isp_hiaddr != curr_hiaddr)
	{
		isp_hiaddr = curr_hiaddr;
		/* Load Extended Address byte */
		spi_fast_shift(0x4D);
		spi_fast_shift(0x00);
		spi_fast_shift(isp_hiaddr);
		spi_fast_shift(0x00);
	}
}

uchar ispReadFlash(unsigned long address) {

	ispUpdateExtended(address);

	spi_fast_shift(0x20 | ((address & 1) << 3));
	spi_fast_shift(address >> 9);
	spi_fast_shift(address >> 1);
	return spi_fast_shift(0);
}

uchar ispWriteFlash(unsigned long address, uchar data, uchar pollmode) {

	/* 0xFF is value after chip erase, so skip programming
	 if (data == 0xFF) {
	 return 0;
	 }
	 */

	ispUpdateExtended(address);

	spi_fast_shift(0x40 | ((address & 1) << 3));
	spi_fast_shift(address >> 9);
	spi_fast_shift(address >> 1);
	spi_fast_shift(data);

	if (pollmode == 0)
		return 0;

	if (data == 0x7F) {
		clockWait(15); /* wait 4,8 ms */
		return 0;
	} else {

		/* polling flash */
		uchar retries = 30;
		uint8_t starttime = TIMERVALUE;
		while (retries != 0) {
			if (ispReadFlash(address) != 0x7F) {
				return 0;
			};

			if ((uint8_t) (TIMERVALUE - starttime) > CLOCK_T_320us) {
				starttime = TIMERVALUE;
				retries--;
			}

		}
		return 1; /* error */
	}

}

uchar ispFlushPage(unsigned long address, uchar pollvalue) {

	ispUpdateExtended(address);
	
	spi_fast_shift(0x4C);
	spi_fast_shift(address >> 9);
	spi_fast_shift(address >> 1);
	spi_fast_shift(0);

	if (pollvalue == 0xFF) {
		clockWait(15);
		return 0;
	} else {

		/* polling flash */
		uchar retries = 30;
		uint8_t starttime = TIMERVALUE;

		while (retries != 0) {
			if (ispReadFlash(address) != 0xFF) {
				return 0;
			};

			if ((uint8_t) (TIMERVALUE - starttime) > CLOCK_T_320us) {
				starttime = TIMERVALUE;
				retries--;
			}

		}

		return 1; /* error */
	}

}

uchar ispReadEEPROM(unsigned int address) {
	spi_fast_shift(0xA0);
	spi_fast_shift(address >> 8);
	spi_fast_shift(address);
	return spi_fast_shift(0);
}

uchar ispWriteEEPROM(unsigned int address, uchar data) {

	spi_fast_shift(0xC0);
	spi_fast_shift(address >> 8);
	spi_fast_shift(address);
	spi_fast_shift(data);

	clockWait(30); // wait 9,6 ms

	return 0;
}
