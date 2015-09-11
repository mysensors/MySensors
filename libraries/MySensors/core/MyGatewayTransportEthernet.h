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
#include "MyGatewayTransport.h"
#include "MyProtocolDefault.h"

#include "drivers/Ethernet_W5100/IPAddress.h"

// Use this if you have attached a Ethernet ENC28J60 shields  
//#include <UIPEthernet.h>  

// Use this fo WizNET W5100 module and Arduino Ethernet Shield 
#include "drivers/Ethernet_W5100/Ethernet.h"

#ifdef MY_USE_UDP
#include "drivers/Ethernet_W5100/EthernetUdp.h"
#endif /* MY_USE_UDP */

#define CONTROLLER_PORT 5003
#define IP_RENEWAL_INTERVAL 60000  // in milliseconds
#define MAX_RECEIVE_LENGTH 100 // Maximum message length for messages coming from controller


class MyGatewayTransportEthernet : public MyGatewayTransport
{
	public:
		MyGatewayTransportEthernet(MyProtocol &protocol,
				uint8_t *gw_mac
#ifndef IP_ADDRESS_DHCP
				, IPAddress gw_ip
#endif /* not IP_ADDRESS_DHCP */
				, uint16_t gw_port
#ifdef IP_ADDRESS_DHCP
				, unsigned long ip_renew_interval
#endif /* IP_ADDRESS_DHCP */
				, IPAddress controller_IP
				, uint16_t controller_port
				);

	// initialize the driver
	bool begin();

	// Send message to controller
	bool send(MyMessage &message);

	// Check if a new message is available from controller
	bool available();

	// Pick up last message received from controller
	MyMessage& receive();

	// Set the controller IP and Port
	void setControllerIPPort(const IPAddress& addr, const uint16_t port);

protected:
	IPAddress controllerIP;
	uint16_t controllerPort;

#ifdef IP_ADDRESS_DHCP
	void renewIP();
	unsigned long ip_renewal_period;
#endif /* IP_ADDRESS_DHCP */
	uint8_t *gatewayMAC;
	IPAddress gatewayIP;
	uint16_t gatewayPort;

#ifdef MY_USE_UDP
	EthernetUDP *server;
#else
	EthernetServer *server;
	uint8_t inputPos;
#endif

	char inputBuffer[MAX_RECEIVE_LENGTH];    // A buffer for incoming commands from serial interface
	MyMessage msg;
};

#endif /* MyGatewayTransportEthernet_h */
