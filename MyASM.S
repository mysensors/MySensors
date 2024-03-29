/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2022 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

#if defined(ARDUINO_ARCH_SAMD)
/* workaround to prevent compiler error */
	.thumb_func
doNothing:
	nop
	.size	doNothing, .-doNothing

#elif defined(ARDUINO_ARCH_NRF5)
/* workaround to prevent compiler error */
	.thumb_func
doNothing:
	nop
	.size	doNothing, .-doNothing

#elif defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_MEGAAVR)

#include "hal/crypto/AVR/drivers/SHA256/SHA256.S"

#endif