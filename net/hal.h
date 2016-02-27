/*
 * Copyright (c) 2012 by Paweł Lebioda <pawel.lebioda89@gmail.com>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _HAL_H
#define _HAL_H

#include "../dev/enc28j60.h"

#define hal_init(mac)	enc28j60_init((mac))

#define hal_send_packet(buff,len) enc28j60_send_packet((buff),(len))

#define hal_receive_packet(buff,max_len,pkts) enc28j60_receive_packet((buff),(max_len),pkts)

#define hal_link_up()	

#define hal_have_packets() 	enc28j60_have_packet()

#if ENC28J60_ENABLE_HW_TX_CHECKSUM
#define hal_crc_tx_add(s,l,p) enc28j60_crc_tx_add(s, l, p)
#endif

#endif //_HAL_H


