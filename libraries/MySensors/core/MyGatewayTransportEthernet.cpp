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

#include "MyGatewayTransportEthernet.h"

IPAddress _ethernetControllerIP(MY_CONTROLLER_IP_ADDRESS);
IPAddress _ethernetGatewayIP(MY_IP_ADDRESS);
byte _ethernetGatewayMAC[] = { MY_MAC_ADDRESS };
uint16_t _ethernetGatewayPort = MY_PORT;
char _ethernetInputBuffer[MAX_RECEIVE_LENGTH];    // A buffer for incoming commands from serial interface
MyMessage _ethernetMsg;

#ifdef MY_USE_UDP
	EthernetUDP _ethernetServer;
#else
	// we have to use pointers due to the constructor of EthernetServer
	EthernetServer _ethernetServer(_ethernetGatewayPort);
	uint8_t _ethernetInputPos;
#endif /* USE_UDP */


bool gatewayTransportInit() {
#ifdef MY_IP_ADDRESS
	Ethernet.begin(_ethernetGatewayMAC, _ethernetGatewayIP);
#else
	// Get IP address from DHCP
	Ethernet.begin(_ethernetGatewayMAC);
#endif /* IP_ADDRESS_DHCP */

	// give the Ethernet interface a second to initialize
	// TODO: use HW delay
	delay(1000);

#ifdef MY_USE_UDP
	_ethernetServer.begin(_ethernetGatewayPort);
#else
	// we have to use pointers due to the constructor of EthernetServer
	_ethernetServer.begin();
#endif /* USE_UDP */
}

bool gatewayTransportSend(MyMessage &message)
{
	char *_ethernetMsg = NULL;

	if (_ethernetControllerIP == INADDR_NONE) {
		// no controller IP address set!
		return false;
	}
	else {
		_ethernetMsg = protocolFormat(message);
#ifdef MY_USE_UDP
		_ethernetServer.beginPacket(_ethernetControllerIP, MY_PORT);
		_ethernetServer.write(_ethernetMsg, strlen(_ethernetMsg));
		// returns 1 if the packet was sent successfully
		return _ethernetServer.endPacket();
#else
		EthernetClient client;
		if (client.connect(_ethernetControllerIP, MY_PORT)) {
			client.write(_ethernetMsg, strlen(_ethernetMsg));
			client.stop();
			return true;
		}
		else {
			// connecting to the server failed!
			return false;
		}
#endif /* USE_UDP */ 
	}
}

bool gatewayTransportAvailable()
{
	bool available = false;

#ifndef MY_IP_ADDRESS
	// renew IP address using DHCP
	gatewayTransportRenewIP();
#endif

#ifdef MY_USE_UDP
	int packet_size = _ethernetServer.parsePacket();

	if (_ethernetServer.available()) {
		_ethernetServer.read(_ethernetInputBuffer, MAX_RECEIVE_LENGTH);
		available = true;
	}
#else
	// if an incoming client connects, there will be
	// bytes available to read via the client object
	EthernetClient client = _ethernetServer.available();

	if (client) {
		while (client.available()) {
			// read the bytes incoming from the client
			char inChar = client.read();

			if (_ethernetInputPos < MAX_RECEIVE_LENGTH - 1) {
				// if newline then command is complete
				if (inChar == '\n') {
					_ethernetInputBuffer[_ethernetInputPos] = 0;
					available = true;
				} else {
					_ethernetInputBuffer[_ethernetInputPos] = inChar;
					_ethernetInputPos++;
				}
			}
		}
	}
	_ethernetInputPos = 0;
#endif

	if (available) {
		// Parse message and return parse result
		available = protocolParse(_ethernetMsg, _ethernetInputBuffer);
	}

	return available;
}

MyMessage& gatewayTransportReceive()
{
	// Return the last parsed message
	return _ethernetMsg;
}


#ifndef MY_IP_ADDRES
void gatewayTransportRenewIP()
{
	/* renew/rebind IP address
	 0 - nothing happened
	 1 - renew failed
	 2 - renew success
	 3 - rebinf failed
	 4 - rebind success
	 */
	// TODO: Use millis from MyHw ??
	static unsigned long next_time = millis() + MY_IP_RENEWAL_INTERVAL;
	unsigned long now = millis();

	// http://playground.arduino.cc/Code/TimingRollover
	if ((long)(now - next_time) < 0)
		return;

	if (Ethernet.maintain() & ~(0x06)) {
		/* Error occured -> IP was not renewed */
		return;
	}

	next_time = now + MY_IP_RENEWAL_INTERVAL;
}
#endif /* IP_ADDRESS_DHCP */
