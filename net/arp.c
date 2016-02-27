/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "arp.h"
// 
#include "../debug.h"

#include <string.h>
#include <avr/pgmspace.h>

struct arp_header
{
	uint16_t 		hardware_type;
	uint16_t 		protocol_type;
	uint8_t 		hardware_addr_len;
	uint8_t 		protocol_addr_len;
	uint16_t 		operation_code;
	ethernet_address 	sender_hardware_addr;
	ip_address 		sender_protocol_addr;
	ethernet_address 	target_hardware_addr;
	ip_address 		target_protocol_addr;
};

#define ARP_TABLE_ENTRY_STATUS_REQUEST	0
#define ARP_TABLE_ENTRY_STATUS_TIMEOUT	1
#define ARP_TABLE_ENTRY_STATUS_EMPTY	0xff 


struct arp_table_entry
{
	uint8_t			status;
	ip_address 		ip_addr;
	ethernet_address 	ethernet_addr;
	uint16_t		timeout;
};

static struct arp_table_entry 	arp_table[ARP_TABLE_SIZE]; //EXMEM

#define FOREACH_ARP_ENTRY(entry) for(entry = &arp_table[0] ; entry < &arp_table[ARP_TABLE_SIZE] ; entry++)


uint8_t arp_send_reply(const struct arp_header * header);

uint8_t arp_init(void)
{
	/* Clear arp table */
	memset(arp_table, 0, sizeof(struct arp_table_entry) * ARP_TABLE_SIZE);
	return 1;
}

uint8_t arp_handle_packet(struct arp_header * header,uint16_t packet_length)
{
	/* Check packet length */
	if(packet_length < sizeof(struct arp_header))
		return 0;
	/* Check hardware address size */
	if(header->hardware_addr_len != ARP_HW_ADDR_SIZE_ETHERNET)
		return 0;
	/* Check protocol address size */
	if(header->protocol_addr_len != ARP_PROTO_ADDR_SIZE_IP)
		return 0;
	/* Check hardware address type */
	if(header->hardware_type != HTON16(ARP_HW_ADDR_TYPE_ETHERNET))
		return 0;
	/* Check protocol address type */
	if(header->protocol_type != HTON16(ARP_PROTO_ADDR_TYPE_IP))
		return 0;		
	/* Check whether target protocol address is our's */
	if(memcmp(header->target_protocol_addr,ip_get_addr(),sizeof(ip_address)))
		return 0;
	/* Parse operation code of packet */
	if(header->operation_code != HTON16(ARP_OPERATION_REQUEST) && header->operation_code != HTON16(ARP_OPERATION_REPLY))
		return 0;
	arp_table_insert((const ip_address*)&header->sender_protocol_addr,(const ethernet_address*)&header->sender_hardware_addr);
	if(header->operation_code == HTON16(ARP_OPERATION_REQUEST))
		return arp_send_reply(header);
	return 1;
}

uint8_t arp_send_reply(const struct arp_header * header)
{
	struct arp_header * arp_reply = (struct arp_header*)ethernet_get_buffer();
	
	/* set hardware address length */
	arp_reply->hardware_addr_len = header->hardware_addr_len;
	/* set hardware address type */
	arp_reply->hardware_type = header->hardware_type;
	/* set protocol address length */
	arp_reply->protocol_addr_len = header->protocol_addr_len;
	/* set protocol address type */
	arp_reply->protocol_type = header->protocol_type;
	/* set operation code to echo reply */
	arp_reply->operation_code = HTON16(ARP_OPERATION_REPLY);
	/* set target's hardware address */
	memcpy(&arp_reply->target_hardware_addr,&header->sender_hardware_addr,sizeof(ethernet_address));
	/* set target's protocol address */
	memcpy(&arp_reply->target_protocol_addr,&header->sender_protocol_addr,sizeof(ip_address));
	/* set our protocol address */
	memcpy(&arp_reply->sender_protocol_addr,ip_get_addr(),sizeof(ip_address));
	/* set our hardware address */
	memcpy(&arp_reply->sender_hardware_addr,ethernet_get_mac(),sizeof(ethernet_address));
	
	return ethernet_send_packet(&arp_reply->target_hardware_addr,ETHERNET_TYPE_ARP,sizeof(struct arp_header));
}

void arp_table_insert(const ip_address * ip_addr,const ethernet_address * ethernet_addr)
{
	uint16_t min_timeout = ARP_TABLE_ENTRY_TIMEOUT;
	struct arp_table_entry * entry;
	struct arp_table_entry * min_entry = 0;
	FOREACH_ARP_ENTRY(entry)
	{
		/* Check if there is already entry in arp table with specified ip addr */
		if(!memcmp(ip_addr,&entry->ip_addr,sizeof(ip_address)))
		{
			/*
			switch(entry->status)
			{
				case ARP_TABLE_ENTRY_STATUS_REQUEST:
				*/
					min_entry = entry;
					goto arp_table_insert_out;
					/*
				case ARP_TABLE_ENTRY_STATUS_TIMEOUT:
					entry->timeout = ARP_TABLE_ENTRY_TIMEOUT;
					return;
				default:
					break;
			}
			*/
		}
	}

	if(min_entry == 0)
	{
		FOREACH_ARP_ENTRY(entry)
		{
			/* find entry with minimum timeout*/
			if(entry->timeout < min_timeout)
			{
				min_entry = entry;
				min_timeout = entry->timeout;
			}
		}
	}
	
	/* there must be some error (maybe arp timer does not work?) */
	if(min_entry == 0)
	{
		return;
	}
arp_table_insert_out:
	/* set default timeout */
	min_entry->timeout = ARP_TABLE_ENTRY_TIMEOUT;
	/* set status to timeout */
	min_entry->status = ARP_TABLE_ENTRY_STATUS_TIMEOUT;	
	/* copy ip address */
	memcpy(&min_entry->ip_addr, ip_addr, sizeof(ip_address));
	/* copy mac address */
	memcpy(&min_entry->ethernet_addr, ethernet_addr, sizeof(ethernet_address));
}

uint8_t arp_get_mac(const ip_address * ip_addr, ethernet_address * ethernet_addr)
{
	if(ip_addr == 0)
		return 0;
		
	struct arp_table_entry * entry;
	struct arp_table_entry * empty = 0;
	FOREACH_ARP_ENTRY(entry)
	{
		if(entry->status == ARP_TABLE_ENTRY_STATUS_EMPTY)
		{
			empty = entry;
			continue;
		}
		
		if(!memcmp(&entry->ip_addr, ip_addr, sizeof(ip_address)))
		{
			switch(entry->status)
			{
				/* There is ethernet address in arp table */
				case ARP_TABLE_ENTRY_STATUS_TIMEOUT:
					entry->timeout = ARP_TABLE_ENTRY_TIMEOUT;
					if(ethernet_addr)
						memcpy(ethernet_addr,&entry->ethernet_addr,sizeof(ethernet_address));
					return 1;
				/* There is ip address but waiting for reply*/
				case ARP_TABLE_ENTRY_STATUS_REQUEST:
					return 0;
				default:
					continue;
			}
		}
	}
	return 0;
}
