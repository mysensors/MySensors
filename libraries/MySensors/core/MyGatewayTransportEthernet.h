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

#ifndef MyGatewayTransportEthernet_h
#define MyGatewayTransportEthernet_h

#include "MyConfig.h"
#include "MyMessage.h"
#include "MyProtocol.h"
#include "MyProtocolMySensors.h"
#include "MyGatewayTransport.h"


// Use this if you have attached a Ethernet ENC28J60 shields  
//#include <UIPEthernet.h>  



#define CONTROLLER_PORT 5003
#define IP_RENEWAL_INTERVAL 60000  // in milliseconds
#define MAX_RECEIVE_LENGTH 100 // Maximum message length for messages coming from controller


#ifndef MY_IP_ADDRESS
	void gatewayTransportRenewIP();
#endif

#endif /* MyGatewayTransportEthernet_h */
