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
 * ATSHA204 emulated signing backend. The emulated ATSHA204 implementation offers pseudo random
 * number generation and HMAC-SHA256 authentication compatible with a "physical" ATSHA204.
 * NOTE: Key is stored in clear text in the Arduino firmware. Therefore, the use of this back-end
 * could compromise the key used in the signed message infrastructure if device is lost and its memory
 * dumped.
 *
 */

#ifndef MySigningHmac256Soft_h
#define MySigningHmac256Soft_h

#include "MyConfig.h"
#include "MySigning.h"
#include "drivers/ATSHA204/ATSHA204.h"
#include "drivers/ATSHA204/sha256.h"
#include <stdint.h>



// This implementation is the pure software variant of the ATSHA204.
// It is designed to work fully compliant with nodes using ATSHA204 in the network
// and therefore uses the same signing identifier as ATSHA204.
// Because it is completely software based, the quality of the generated random numbers
// is weaker though. Random numbers are generated using the Arduino library and seed
// is sampled from an analog pin. This pin should unconnected in the hardware.
// The pin is selected using MY_SIGNING_SOFT_RANDOMSEED_PIN in MyConfig.h.

void signerCalculateSignature(MyMessage &msg);


#endif
