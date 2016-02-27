#pragma once

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <string.h>
#include <stdio.h>

#include "config/config.h"

#include "arch/spi.h"
#include "dev/enc28j60.h"
#include "dev/as110x.h"

#include "net/hal.h"
#include "net/ethernet.h"
#include "net/ip.h"
#include "net/arp.h"

