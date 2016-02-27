#pragma once

#include "net/ip.h"

struct config
{
	uint16_t cfgSize;

	ethernet_address netMac;
	
	uint8_t 	net;
	ip_address 	netIp;
	ip_address 	netMask;
	ip_address 	netGateway;
	ip_address 	netDns;
};

extern struct config gConfig; 

#define CONFIG_NET_ENABLE_DHCP 	0x01
#define CONFIG_NET_ALT_CONFIG 	0x02
#define CONFIG_SAVE_DHCP_AS_ALT	0x04

void config_read();
void config_write();