/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/**
 * @addtogroup net
 * @{
 * @addtogroup ip
 * @{
 * @file ip.c
 * @author Pawel Lebioda <pawel.lebioda89@gmail.com>
 * @brief This file contains definition of Internet Protocol layer functions
 */
#include <string.h>
#include <stdio.h>

#include "ethernet.h"
#include "ip.h"
#include "net.h"
#include "arp.h"
#include "icmp.h"
#include "hal.h"

#include "../debug.h"

extern void onUDP(uint8_t * data, uint16_t len, const ip_address *from, const uint16_t port);

struct udp_header
{
	uint16_t port_source;
	uint16_t port_destination;
	uint16_t length;
	uint16_t checksum;
};

/**
 * Internet Protocol version 4
 */
#define IP_V4		0x4

#define IP_VIHL_HL_MASK	0xf
/**
 * Internet Protocol Header
 */
struct ip_header
{
	/**
 	 * Version and header length
	 */
	union
	{
		/**
		 * Version - 4 bits
		 */
		uint8_t version; 

		/**
 		 * Header length in 4 bytes words - 4 bits
		 */
		uint8_t header_length;
	} vihl;

	/**
	 * Type of service
	 */
	uint8_t 	type_of_service;
	
	/**
	 * Data length
	 */
	uint16_t 	length;

	/**
	 * Packet ID
	 */
	uint16_t 	id;
	
	/**
	 * Flags and fragment offset
	 */
	union
	{
		/**
		 * Flags
		 */
		uint16_t flags;
		
		/**
		 * Fragment offset
 		 */
		uint16_t fragment_offset;
	} ffo;
	
	/**
	 * More fragments
	 */
	#define IP_FLAGS_MORE_FRAGMENTS 0x01

	/**
	 * Do not fragment
	 */
	#define IP_FLAGS_DO_NOT_FRAGMENT 0x02

	/**
	 * Time to live
	 */
	uint8_t 	ttl;
	
	/**
	 * Protocol
	 */
	uint8_t 	protocol;
	
	/**
	 * Checksum
	 */
	uint16_t 	checksum;

	/**
	 * Source IP Address
	 */
	ip_address	src;

	/**
	 * Destination IP Address
	 */
	ip_address 	dst;
};

/**
 * IP Address
 */
static ip_address ip_addr = NET_IP_ADDRESS;

const ip_address * ip_get_addr(void)		{return (const ip_address*)&ip_addr;} 

/**
 *
 */
void ip_init(const ip_address * addr,const ip_address * netmask,const ip_address * gateway)
{
	memcpy(&ip_addr,addr,sizeof(ip_address));
}

/**
 *
 */
uint8_t ip_send_packet(const ip_address * ip_dst,uint8_t protocol,uint16_t length)
{
	ethernet_address mac;
	
	/* otherwise try to get mac form arp table */
	if(!arp_get_mac(ip_dst, &mac)) // try arp table first, this could be external gateway address
	{
		memset(&mac, 0xff, sizeof(ethernet_address));
	}
	
	struct ip_header * ip = (struct ip_header*)ethernet_get_buffer();
	
	/* clear ip header */
	memset(ip,0,sizeof(struct ip_header));
	
	/* set version */
	ip->vihl.version = (IP_V4)<<4;
	
	/* set header length */
	ip->vihl.header_length |= (sizeof(struct ip_header) / 4) & 0xf;
	
	/* set ip packet length */
	uint16_t total_len = (uint16_t)sizeof(struct ip_header) + length;
	ip->length = hton16(total_len);
	
	/* set time to live */
	ip->ttl = 2;
	
	/* set protocol */
	ip->protocol = protocol;
	
	/* set src addr */
	memcpy(&ip->src,ip_get_addr(),sizeof(ip_address));
	
	/* set dst addr */
	memcpy(&ip->dst,ip_dst,sizeof(ip_address));
	
	/* compute checksum */
	ip->checksum = hton16(~net_get_checksum(0,(const uint8_t*)ip,sizeof(struct ip_header),10));

	/* send packet */
	return ethernet_send_packet(&mac, ETHERNET_TYPE_IP, total_len);
}
/**
 *
 */
uint8_t ip_handle_packet(struct ip_header * header, uint16_t packet_len,const ethernet_address * mac )
{	
	if(packet_len < sizeof(struct ip_header))
		return 0;

	/* Check IP version */
	if((header->vihl.version>>4) != IP_V4)
		return 0;

	/* get header length */
	uint8_t header_length = (header->vihl.header_length & IP_VIHL_HL_MASK)*4;
	
	/* get packet length */
	uint16_t packet_length = ntoh16(header->length);
	
	/* check packet length */
	if(packet_length > packet_len)
		return 0;

	/* do not support fragmentation */
	//if(ntoh16(header->ffo.flags) & (IP_FLAGS_MORE_FRAGMENTS << 13) || ntoh16(header->ffo.fragment_offset) & 0x1fff)
	//	return 0;
		
	/* check destination ip address */
	if(memcmp(&header->dst,ip_get_addr(),sizeof(ip_address)))
	{
		return 0;
	}

	/* check checksum */
	if(ntoh16(header->checksum) != (~net_get_checksum(0,(const uint8_t*)header,header_length,10)))
		return 0;

	/* add to arp table if ip does not exist */
	arp_table_insert((const ip_address*)&header->src, mac);
	
	packet_len = packet_length - header_length;
	/* redirect packet to the proper upper layer */
	switch(header->protocol)
	{
#if NET_ICMP		
		case IP_PROTOCOL_ICMP:
			icmp_handle_packet(
				(const ip_address*)&header->src,
				(const struct icmp_header*)((const uint8_t*)header + header_length),
				packet_len);
			break;
#endif //NET_ICMP
		case IP_PROTOCOL_UDP:
		{
			const struct udp_header *udp = (const struct udp_header*)((const uint8_t*)header + header_length);
			if(packet_len < sizeof(struct udp_header))
				break;
		
			if (~net_get_checksum(IP_PROTOCOL_UDP + packet_len, ((const uint8_t*)udp) - 8, packet_len + 8, 0x0FFF))
			{
				break;
			}
		
			/* get local port number */
			uint16_t port_local = ntoh16(udp->port_destination);
			/* get remote port number */
			uint16_t port_remote = ntoh16(udp->port_source);
			/* check port and remote ports*/

			if (port_local != 69)
				return 0;
		
			onUDP(
				((uint8_t*)udp) + sizeof(struct udp_header),
				packet_len - sizeof(struct udp_header),
				(const ip_address*)&header->src,
				port_remote
				);
			break;
		}
		default:
			break;
	}
	return 0;
}

uint8_t udp_send_to(uint16_t length, const ip_address * ip_remote, const uint16_t port)
{
	length += sizeof(struct udp_header);

	struct udp_header * udp = (struct udp_header*)ip_get_buffer();
	udp->length = hton16(length);
	udp->port_destination = hton16(port);
	udp->port_source = hton16(69);

	udp->checksum = 0;//hton16(udp_get_checksum((const ip_address*)ip_remote, udp, length));
	return ip_send_packet(ip_remote, IP_PROTOCOL_UDP, length);
}
