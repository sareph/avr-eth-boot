/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define ENC28J60_CS_PORT	PORTC
#define ENC28J60_CS_DDR		DDRC
#define ENC28J60_CS			0
#define ENC28J60_RST_PORT	PORTE
#define ENC28J60_RST_DDR	DDRE
#define ENC28J60_RST		3

//#define ENC28J60_INT_PORT	PORTB
//#define ENC28J60_INT_DDR	DDRB
//#define ENC28J60_INT		0

#define ENC28J60_NO_HW_RESET 	1

#define ENC28J60_FULL_DUPLEX	0

#define ENC28J60_ENABLE_HW_TX_CHECKSUM 0
#define ENC28J60_ENABLE_HW_RX_CHECKSUM 0 // unimplemented

#define	MAX_FRAMELEN		 	(512+128)