/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _IP_H
#define _IP_H

#include <stdint.h>

#include <avr/pgmspace.h>

#include "ethernet.h"

#define IP_PROTOCOL_ICMP	1
#define IP_PROTOCOL_UDP		17
#define IP_PROTOCOL_TCP		6

struct udp_header;

typedef uint8_t ip_address[4];


/**
 *
 */
struct ip_header;

/**
 * Inits IP module
 * @param [in] addr IP Address
 * @param [in] netmask Netmask
 * @param [in] gateway Gateway
 */
void ip_init(const ip_address * addr,const ip_address * netmask,const ip_address * gateway);

/**
 */
const ip_address * ip_get_addr(void);

/**
 *
 */
const ip_address * ip_get_netmask(void);

/**
 *
 */
const ip_address * ip_get_broadcast(void); 

/**
 *
 */
const ip_address * ip_get_gateway(void);

/**
 *
 */
const char * ip_addr_str(const ip_address * addr);

/**
 *
 */
const char * ip_addr_port_str(const ip_address * addr,uint16_t portno);

/**
 *
 */
uint8_t ip_send_packet(const ip_address * ip_dst,uint8_t protocol,uint16_t length);

/**
 *
 */
uint8_t ip_handle_packet(struct ip_header * header, uint16_t packet_len,const ethernet_address * mac );

uint8_t udp_send_to(uint16_t length, const ip_address * ip_remote, const uint16_t port);

uint8_t ip_is_broadcast(const ip_address * ip);

#define ip_get_buffer() (ethernet_get_buffer() + NET_HEADER_SIZE_IP)
#define ip_get_buffer_size() (ethernet_get_buffer_size() - NET_HEADER_SIZE_IP)
#define udp_get_buffer() (ip_get_buffer() + NET_HEADER_SIZE_UDP)
#define udp_get_buffer_size() (ip_get_buffer_size() - NET_HEADER_SIZE_UDP)

#endif //_IP_H


