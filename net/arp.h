/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ARP_H
#define _ARP_H

#include <stdint.h>
#include <stdio.h>

#include "ip.h"
#include "ethernet.h"
#include <config/net_arp.h>

struct arp_header;

#define ARP_HW_ADDR_TYPE_ETHERNET	0x0001
#define ARP_HW_ADDR_SIZE_ETHERNET	0x06
#define ARP_PROTO_ADDR_TYPE_IP		0x0800
#define ARP_PROTO_ADDR_SIZE_IP		0x04
#define ARP_OPERATION_REQUEST		0x0001
#define ARP_OPERATION_REPLY		0x0002

uint8_t arp_init(void);
uint8_t arp_handle_packet(struct arp_header * header,uint16_t packet_length);
uint8_t arp_get_mac(const ip_address * ip_addr,ethernet_address * ethernet_addr);
void arp_table_insert(const ip_address * ip_addr,const ethernet_address * ethernet_addr);
void arp_print_stat(FILE * fh);

#endif //_ARP_H