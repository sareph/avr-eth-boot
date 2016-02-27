/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "enc28j60.h"

#include <avr/io.h>
#include <avr/interrupt.h>
static uint8_t	enc28j60_bank;
static uint16_t enc28j60_next_packet_ptr;

uint8_t enc28j60_read_op(uint8_t op,uint8_t addr)
{
//	 SPI_ENABLE();
	ENC28J60_CS_ACTIVE();
	spi_fast_shift(op | (addr & ENC28J60_ADDR_MASK));
	uint8_t ret;
	ret = spi_fast_shift(0x00);
	// do dummy read if needed (for mac and mii, see datasheet page 29)
	if(addr & 0x80)
	{
		ret = spi_fast_shift(0x00);
	}
	ENC28J60_CS_INACTIVE();
	return ret;
}


void enc28j60_write_op(uint8_t op,uint8_t addr,uint8_t data)
{
	ENC28J60_CS_ACTIVE();

	spi_fast_shift(op | (addr & ENC28J60_ADDR_MASK));
	spi_fast_shift(data);
	
	ENC28J60_CS_INACTIVE();
}

void enc28j60_read_buffer(uint16_t len,uint8_t * data)
{
	ENC28J60_CS_ACTIVE();
	spi_fast_shift(ENC28J60_OPC_RBM);
	spi_transfer_sync(data, data, len);

	data[len] = '\0';
	
	ENC28J60_CS_INACTIVE();
}

void enc28j60_write_buffer(uint16_t len,uint8_t * data)
{			 
	ENC28J60_CS_ACTIVE();
	
	spi_fast_shift(ENC28J60_OPC_WBM);
	spi_transfer_sync(data, data, len);
	
	ENC28J60_CS_INACTIVE();
}

void enc28j60_set_bank(uint8_t addr)
{
	uint8_t address = (addr & ENC28J60_BANK_MASK);
	if( address != enc28j60_bank)
	{
		enc28j60_write_op(ENC28J60_OPC_BFC,ECON1,(ECON1_BSEL0|ECON1_BSEL1));
		enc28j60_write_op(ENC28J60_OPC_BFS,ECON1,address>>5);
		enc28j60_bank = address;
	}
}

uint8_t enc28j60_read(uint8_t addr)
{
	enc28j60_set_bank(addr);
	return enc28j60_read_op(ENC28J60_OPC_RCR,addr);
}

void enc28j60_write(uint8_t addr,uint8_t data)
{
	enc28j60_set_bank(addr);
	enc28j60_write_op(ENC28J60_OPC_WCR,addr,data);
}

void enc28j60_phy_write(uint8_t addr,uint16_t data)
{
	//set the PHY register address
	enc28j60_write(MIREGADR,addr);
	//write the PHY data
	enc28j60_write(MIWRL,data);
	enc28j60_write(MIWRH,data>>8);
	//wait until the PHY write completes	
	while(enc28j60_read(MISTAT) & MISTAT_BUSY)
	{
		_delay_us(15);
	}	
}

uint16_t enc28j60_phy_read(uint8_t addr)
{
	//set the PHY register address
	enc28j60_write(MIREGADR, addr);
	//write the PHY cmd
	enc28j60_write(MICMD, MICMD_MIIRD);
	//wait until the PHY read completes
	while(enc28j60_read(MISTAT) & MISTAT_BUSY)
	{
		_delay_us(15);
	}
		
	uint16_t r = enc28j60_read(MIRDL);
	uint16_t h = enc28j60_read(MIRDH);
	
	enc28j60_write(MICMD, 0);

	r |= h << 8;	
	return r;
	
}

void enc28j60_clear_bits(uint8_t address,uint8_t bits)
{
	uint8_t reg = enc28j60_read(address);
	reg &= ~bits;
	enc28j60_write(address,reg);
}

void enc28j60_set_bits(uint8_t address,uint8_t bits)
{
	uint8_t reg = enc28j60_read(address);
	reg |= bits;
	enc28j60_write(address,reg);	
}
void enc28j60_io_init(void)
{
	ENC28J60_CS_DDR |= (1<<ENC28J60_CS);
#if ENC28J60_NO_HW_RESET == 0
	ENC28J60_RST_DDR |= (1<<ENC28J60_RST);
	ENC28J60_RST_OFF();
#endif
}

void enc28j60_reset(void)
{
#if ENC28J60_NO_HW_RESET == 0
	ENC28J60_RST_ON();
	_delay_ms(10);
	ENC28J60_RST_OFF();
	_delay_ms(10);
#endif
}
void enc28j60_init(const uint8_t * macaddr, link_status_callback cb)
{	
	
	//DBG_("enc28j60t init:\n");
	//DBG_("%c:%c:%c:%c:%c:%c\n",(macaddr)[0],(macaddr)[1],(macaddr)[2],(macaddr)[3],(macaddr)[4],(macaddr)[5]);
	//DBG_("\n");

	enc28j60_io_init();	
	enc28j60_reset();

	ENC28J60_CS_INACTIVE();
	enc28j60_write_op(ENC28J60_OPC_RES,0,ENC28J60_OPC_RES);
	_delay_ms(50);
	
	// BANK 0 STUFF
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	enc28j60_next_packet_ptr = ENC28J60_RXSTART_INIT;
	//Rx start
	enc28j60_write(ERXSTL,ENC28J60_RXSTART_INIT&0xff);
	enc28j60_write(ERXSTH,ENC28J60_RXSTART_INIT>>8);
	//set receive pointer address
	enc28j60_write(ERXRDPTL,ENC28J60_RXSTART_INIT&0xff);
	enc28j60_write(ERXRDPTH,ENC28J60_RXSTART_INIT>>8);
	//Rx end
	enc28j60_write(ERXNDL,ENC28J60_RXSTOP_INIT&0xff);
	enc28j60_write(ERXNDH,ENC28J60_RXSTOP_INIT>>8);
	//Tx start
	enc28j60_write(ETXSTL,ENC28J60_TXSTART_INIT&0xff);
	enc28j60_write(ETXSTH,ENC28J60_TXSTART_INIT>>8);
	//Tx end
	enc28j60_write(ETXNDL,ENC28J60_TXSTOP_INIT&0xff);
	enc28j60_write(ETXNDH,ENC28J60_TXSTOP_INIT>>8);
	
	// BANK 1 STUFF
	// For broadcast packets we allow only ARP packtets
	// All other packets should be unicast only for our mac (MAADR)
	//
	// The pattern to match on is therefore
	// Type		 ETH.DST
	// ARP			BROADCAST
	// 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
	// in binary these poitions are:11 0000 0011 1111
	// This is hex 303F->EPMM0=0x3f,EPMM1=0x30
//	 enc28j60_write(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
//	 enc28j60_write(EPMM0, 0x3f);
//	 enc28j60_write(EPMM1, 0x30);
//	 enc28j60_write(EPMCSL, 0xf9);
//	 enc28j60_write(EPMCSH, 0xf7);
//	/* configure frame filters */
	enc28j60_write(ERXFCON, (ERXFCON_UCEN)	| /* accept unicast packets */
				(ERXFCON_CRCEN) | /* accept packets with valid CRC only */
// 				(0 << ERXFCON_PMEN)	| /* no pattern matching */
// 				(0 << ERXFCON_MPEN)	| /* ignore magic packets */
// 				(0 << ERXFCON_HTEN)	| /* disable hash table filter */
// 				(0 << ERXFCON_MCEN)	| /* ignore multicast packets */
				(ERXFCON_BCEN)	 /* accept broadcast packets */
// 				(0 << ERXFCON_ANDOR)	 /* packets must meet at least one criteria */
		);
	// BANK 2 STUFF
	// enable MAC receive
	enc28j60_write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60_write(MACON2, 0x00);
	// enable automatic padding to 60bytes and CRC operations
	enc28j60_write_op(ENC28J60_OPC_BFS, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set inter-frame gap (non-back-to-back)
	enc28j60_write(MAIPGL, 0x12);
	enc28j60_write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60_write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	// Do not send packets longer than MAX_FRAMELEN:
	enc28j60_write(MAMXFLL, (ENC28J60_MAX_FRAMELEN) & 0xFF);				
	enc28j60_write(MAMXFLH, (ENC28J60_MAX_FRAMELEN) >> 8);

	// BANK 3 STUFF
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60_write(MAADR5, macaddr[0]);
	enc28j60_write(MAADR4, macaddr[1]);
	enc28j60_write(MAADR3, macaddr[2]);
	enc28j60_write(MAADR2, macaddr[3]);
	enc28j60_write(MAADR1, macaddr[4]);
	enc28j60_write(MAADR0, macaddr[5]);
	// no loopback of transmitted frames
	enc28j60_phy_write(PHCON2, PHCON2_HDLDIS);
	// set PHY module control register
	enc28j60_phy_write(PHLCON,(PHLCON_LED_LS_RA<<PHLCON_LEDA)|
					(PHLCON_LED_TA<<PHLCON_LEDB)|
					(PHLCON_STRCH_TIME_73MS<<PHLCON_STRCH_TIME)| // 73ms
					(1<<PHLCON_STRCH)
					);
	
	// switch to bank 0
	enc28j60_set_bank(ECON1);
	// enable interrutps
//	 enc28j60_phy_write(PHIE,(1<<PHIE_PLNKIE)|(1<<PHIE_PGEIE));
	enc28j60_write_op(ENC28J60_OPC_BFS, EIE, EIE_INTIE | EIE_PKTIE);
	// enable packet reception
	enc28j60_write_op(ENC28J60_OPC_BFS, ECON1, ECON1_RXEN);
}

uint8_t	enc28j60_get_revision(void)
{
	return enc28j60_read(EREVID);
}

uint8_t	enc28j60_send_packet(uint8_t * packet, uint16_t len)
{
	/* wait until previous packet was sent */
//	 while(enc28j60_read(ECON1) & (1 << ECON1_TXRTS));
	// Set the write pointer to start of transmit buffer area
	enc28j60_write(EWRPTL,ENC28J60_TXSTART_INIT&0xff);
	enc28j60_write(EWRPTH,ENC28J60_TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given 
	enc28j60_write(ETXNDL,(ENC28J60_TXSTART_INIT+len)&0xff);
	enc28j60_write(ETXNDH,(ENC28J60_TXSTART_INIT+len)>>8);
	// write per-packet control byte (0x00 means use macon3 settings)
	enc28j60_write_op(ENC28J60_OPC_WBM, 0, 0x00);
	// copy the packet into the transmit buffer
	enc28j60_write_buffer(len,packet);
	
	// send the contents of the transmit buffer onto the network
	enc28j60_write_op(ENC28J60_OPC_BFS, ECON1, ECON1_TXRTS);
	// Reset the transmit logic problem. See Rev. B4 Silicon Errata point 12.
	if( (enc28j60_read(EIR) & EIR_TXERIF) ){
		enc28j60_write_op(ENC28J60_OPC_BFC, ECON1, ECON1_TXRTS);
	}
	return 1;
}

uint16_t enc28j60_receive_packet(uint8_t * packet,uint16_t maxlen, uint8_t *pkts)
{
	uint16_t rxstatus;
	uint16_t rxlen = 0;
	
	//uint8_t eir = enc28j60_read(EIR);
	enc28j60_read(EIR);
		
	// check if a packet has been received and buffered
	//if( !(enc28j60Read(EIR) & EIR_PKTIF) ){
	// The above does not work. See Rev. B4 Silicon Errata point 6.
	if( !enc28j60_read(EPKTCNT) )
	{
		goto enc28j60_receive_packet_no_packet;
	}
		
	// Set the read pointer to the start of the received packet
	enc28j60_write(ERDPTL,enc28j60_next_packet_ptr&0xff);
	enc28j60_write(ERDPTH,enc28j60_next_packet_ptr>>8);
	
	// read the next packet pointer
	enc28j60_next_packet_ptr	 = enc28j60_read_op(ENC28J60_OPC_RBM,0);
	enc28j60_next_packet_ptr	|= enc28j60_read_op(ENC28J60_OPC_RBM,0)<<8;
	
	// read the packet length (see datasheet page 43)
	rxlen	= enc28j60_read_op(ENC28J60_OPC_RBM,0);
	rxlen |= enc28j60_read_op(ENC28J60_OPC_RBM,0)<<8;
	//remove the CRC count
	rxlen -= 4; 
	
	// read the receive status (see datasheet page 43)
	rxstatus	= enc28j60_read_op(ENC28J60_OPC_RBM,0);
	rxstatus |= enc28j60_read_op(ENC28J60_OPC_RBM,0)<<8;
	
	if(rxlen > maxlen)
		rxlen = maxlen -1;
	
	if( (rxstatus & 0x80) == 0)
		rxlen = 0;
	else
		enc28j60_read_buffer(rxlen,packet);
	
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60_write(ERXRDPTL, (enc28j60_next_packet_ptr));
	enc28j60_write(ERXRDPTH, (enc28j60_next_packet_ptr)>>8);
	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out.
	// However, compensate for the errata point 13, rev B4: enver write an even address!
	if ((enc28j60_next_packet_ptr - 1 < ENC28J60_RXSTART_INIT) || (enc28j60_next_packet_ptr -1 > ENC28J60_RXSTOP_INIT))
	{
		enc28j60_write(ERXRDPTL, (ENC28J60_RXSTOP_INIT)&0xFF);
		enc28j60_write(ERXRDPTH, (ENC28J60_RXSTOP_INIT)>>8);
	} 
	else
	{
		enc28j60_write(ERXRDPTL, (enc28j60_next_packet_ptr-1)&0xFF);
		enc28j60_write(ERXRDPTH, (enc28j60_next_packet_ptr-1)>>8);
	}
	// decrement the packet counter indicate we are done with this packet
	enc28j60_write_op(ENC28J60_OPC_BFS, ECON2, ECON2_PKTDEC);
	enc28j60_receive_packet_no_packet:
	*pkts = enc28j60_have_packet();
	return rxlen;
}

uint8_t enc28j60_have_packet()
{
	return (enc28j60_read(EPKTCNT));
}

void enc28j60_init_clock()
{
	enc28j60_write(ECOCON, 0x02);
}