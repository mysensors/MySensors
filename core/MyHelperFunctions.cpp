/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHelperFunctions.h"

static uint8_t convertH2I(const char c)
{
	if (c <= '9') {
		return c - '0';
	} else if (c >= 'a') {
		return c - 'a' + 10;
	} else {
		return c - 'A' + 10;
	}
}

static char convertI2H(const uint8_t i)
{
	const uint8_t k = i & 0x0F;
	if (k <= 9) {
		return '0' + k;
	} else {
		return 'A' + k - 10;
	}
}
