#include <avr/eeprom.h>
#include "config.h"
#include <config/config.h>
#include <config/net.h>
#include <stdlib.h>

struct config gConfig = {
	sizeof(struct config),
	{0x00, 'K', '2', 0, 0, 0},
	CONFIG_NET_ENABLE_DHCP | CONFIG_NET_ALT_CONFIG | CONFIG_SAVE_DHCP_AS_ALT,
	NET_IP_ADDRESS,		// ip
	NET_IP_NETMASK,		// mask
	NET_IP_GATEWAY,		// gateway
	NET_DNS_SERVER,		// dns

};

void config_read()
{
	struct config tmp;
	eeprom_read_block(&tmp, CONFIG_EEPROM_NET, sizeof(struct config));
	if (tmp.cfgSize == sizeof(struct config))
	{
		gConfig = tmp;
	}
}
