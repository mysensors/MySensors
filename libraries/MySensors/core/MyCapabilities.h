/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Tomas Hozza <thozza@gmail.com>
 * Copyright (C) 2015  Tomas Hozza
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */


#ifndef MyCapabilities_h
#define MyCapabilities_h

#if defined(MY_DISABLE_REMOTE_RESET)
	#define MY_CAP_RESET "N"
#else
	#define MY_CAP_RESET "R"
#endif

#if defined(MY_OTA_FIRMWARE_FEATURE)
	#define MY_CAP_OTA_FW "O"
#else
	#define MY_CAP_OTA_FW "N"
#endif


#if defined(MY_RADIO_NRF24)
	#define MY_CAP_RADIO "N"
#elif defined(MY_RADIO_RFM69)
	#define MY_CAP_RADIO "R"
#elif defined(MY_RS485)
	#define MY_CAP_RADIO "S"
#else
	#define MY_CAP_RADIO "-"
#endif

#if defined(MY_GATEWAY_FEATURE)
	#define MY_CAP_TYPE "G"
#elif defined(MY_REPEATER_FEATURE)
	#define MY_CAP_TYPE "R"
#else
	#define MY_CAP_TYPE "N"
#endif

#if defined(ARDUINO_ARCH_SAMD)
	#define MY_CAP_ARCH "S"
#elif defined(ARDUINO_ARCH_ESP8266)
	#define MY_CAP_ARCH "E"
#elif defined(ARDUINO_ARCH_AVR)
	#define MY_CAP_ARCH "A"
#else
	#define MY_CAP_ARCH "-"
#endif

#if defined(MY_SIGNING_ATSHA204)
	#define MY_CAP_SIGN "A"
#elif defined(MY_SIGNING_SOFT)
	#define MY_CAP_SIGN "S"
#else
	#define MY_CAP_SIGN "-"
#endif


#define MY_CAPABILITIES MY_CAP_RESET MY_CAP_RADIO MY_CAP_OTA_FW MY_CAP_TYPE MY_CAP_ARCH MY_CAP_SIGN

#endif /* MyGatewayTransportEthernet_h */
