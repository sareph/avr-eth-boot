/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _NET_CONFIG_H
#define _NET_CONFIG_H

#define NET_ICMP	0 // 0.2k of code

#define NET_IP_ADDRESS	{10,0,4,1}
#define NET_IP_NETMASK	{255,252,0,0}
#define NET_IP_GATEWAY	{10,0,0,2}
#define NET_DNS_SERVER	{8,8,8,8}

#define NET_IP_BROADCAST_IF_NOT_IN_ARP_TABLE 1

#endif //_NET_CONFIG_H 
