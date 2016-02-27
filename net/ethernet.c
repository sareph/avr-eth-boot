/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "ethernet.h"
#include "hal.h"
#include "ip.h"
#include "arp.h"
// 
#include "../debug.h"

#include <string.h>
#include <stdio.h>

struct ethernet_header
{
	ethernet_address 	dst;
	ethernet_address 	src;
	uint16_t 		type;
};

static ethernet_address ethernet_mac;

uint8_t ethernet_tx_buffer[ETHERNET_MAX_PACKET_SIZE_TX + NET_HEADER_SIZE_ETHERNET]; // EXMEM
uint8_t ethernet_rx_buffer[ETHERNET_MAX_PACKET_SIZE_RX + NET_HEADER_SIZE_ETHERNET]; //EXMEM

void ethernet_init(const ethernet_address * mac)
{
	memcpy(&ethernet_mac,mac,sizeof(ethernet_mac));
}

const ethernet_address * ethernet_get_mac()
{
	return (const ethernet_address*)&ethernet_mac;
}

uint8_t ethernet_handle_packet()
{
	/* receive packet */
	uint8_t pkts = 0;
	uint16_t packet_size = hal_receive_packet(ethernet_rx_buffer, sizeof(ethernet_rx_buffer), &pkts);

	if(packet_size < 1)
	{
		return pkts;
	}
	/* get ethernet header */
	struct ethernet_header * header = (struct ethernet_header*)ethernet_rx_buffer;
	
	/* remove ethernet header from packet for upper layers */
	packet_size -= sizeof(*header);
	uint8_t * data = (uint8_t*)(header+1);	
	
	/* redirect packet to the proper upper layer */
	uint8_t ret=1;
	switch(header->type)
	{
		case HTON16(ETHERNET_TYPE_IP):
			ret = ip_handle_packet((struct ip_header*)data, packet_size, (const ethernet_address*)&header->src);
			break;
		case HTON16(ETHERNET_TYPE_ARP):
			ret = arp_handle_packet((struct arp_header*)data,packet_size);
			break;
		default:
			return pkts;
	}
	return pkts;
}

uint8_t ethernet_send_packet(ethernet_address * dst,uint16_t type,uint16_t len)
{
	if(len > ETHERNET_MAX_PACKET_SIZE_TX - NET_HEADER_SIZE_ETHERNET)
		return 0;
		
	struct ethernet_header * header = (struct ethernet_header*)ethernet_tx_buffer;
	if(dst == ETHERNET_ADDR_BROADCAST)
	{
		memset(&header->dst, 0xff, sizeof(ethernet_address));
	}
	else
	{
		memcpy(&header->dst, dst, sizeof(ethernet_address));
	}
	memcpy(&header->src,&ethernet_mac,sizeof(ethernet_address));
	header->type = hton16(type);

	return hal_send_packet(ethernet_tx_buffer,(len + NET_HEADER_SIZE_ETHERNET));			
}
