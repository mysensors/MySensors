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

#ifndef MyGatewayTransport_h
#define MyGatewayTransport_h

#include "MyProtocol.h"
#include "MySensorsCore.h"

// Common gateway functions

void gatewayTransportProcess();


// Gateway "interface" functions

/**
 * initialize the driver
 */
bool gatewayTransportInit();

/**
 * Send message to controller
 */
bool gatewayTransportSend(MyMessage &message);

/*
 * Check if a new message is available from controller
 */
bool gatewayTransportAvailable();

/*
 * Pick up last message received from controller
 */
MyMessage& gatewayTransportReceive();

#endif /* MyGatewayTransportEthernet_h */
