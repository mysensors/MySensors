/*
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
 */
#define MY_DEBUG
#define MY_DEBUG_VERBOSE_SIGNING
#define MY_RADIO_NRF24
#define MY_SIGNING_SOFT
//#define MY_SIGNING_ATSHA204
#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}}}
#define MY_SIGNING_REQUEST_SIGNATURES
#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7
#define MY_SIGNING_ATSHA204_PIN 17

#include <SPI.h>
#include <MySensor.h>
