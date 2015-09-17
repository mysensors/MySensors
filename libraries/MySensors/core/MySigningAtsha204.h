/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * DESCRIPTION
 * Signing support created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
 * ATSHA204 signing backend. The Atmel ATSHA204 offers true random number generation and
 * HMAC-SHA256 authentication with a readout-protected key.
 *
 */

#ifndef MySigningAtsha204_h
#define MySigningAtsha204_h

#include "MyConfig.h"
#include "MySigning.h"
#include "drivers/ATSHA204/ATSHA204.h"
#include <stdint.h>



// The ATSHA204 is capable of generating proper random numbers for nonce
// and can calculate HMAC-SHA256 signatures. This is enterprise-
// level of security and ought to implement the signing needs for anybody.

void signerCalculateSignature(MyMessage &msg);
uint8_t* signerSha256(const uint8_t* data, size_t sz);


#endif
