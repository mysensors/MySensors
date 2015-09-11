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

MyGatewayTransportEthernet::MyGatewayTransportEthernet(
		MyProtocol &protocol,
		uint8_t *gw_mac
#ifndef IP_ADDRESS_DHCP
		, IPAddress gw_ip
#endif /* not IP_ADDRESS_DHCP */
		, uint16_t gw_port
#ifdef IP_ADDRESS_DHCP
		, unsigned long ip_renew_interval
#endif /* IP_ADDRESS_DHCP */
		, IPAddress controller_IP
		, uint16_t controller_port) :
	MyGatewayTransport(protocol),
	gatewayMAC(gw_mac),
#ifndef IP_ADDRESS_DHCP
	gatewayIP(gw_ip),
#endif /* not IP_ADDRESS_DHCP */
	gatewayPort(gw_port),
#ifdef IP_ADDRESS_DHCP
	ip_renewal_period(ip_renew_interval),
#endif /* IP_ADDRESS_DHCP */
	controllerIP(controller_IP),
	controllerPort(controller_port)
{
	;
}

bool MyGatewayTransportEthernet::begin() {
#ifdef IP_ADDRESS_DHCP
	// Get IP address from DHCP
	Ethernet.begin(gatewayMAC);
#else
	Ethernet.begin(gatewayMAC, gatewayIP);
#endif /* IP_ADDRESS_DHCP */

	// give the Ethernet interface a second to initialize
	// TODO: use HW delay
	delay(1000);

#ifdef MY_USE_UDP
	server = new EthernetUDP();
	server->begin(gatewayPort);
#else
	// we have to use pointers due to the constructor of EthernetServer
	server = new EthernetServer(gatewayPort);
	server->begin();
#endif /* USE_UDP */
}

bool MyGatewayTransportEthernet::send(MyMessage &message)
{
	char *msg = NULL;

	if (controllerIP == INADDR_NONE) {
		// no controller IP address set!
		return false;
	}
	else {
		msg = protocol.format(message);
#ifdef MY_USE_UDP
		server->beginPacket(controllerIP, controllerPort);
		server->write(msg, strlen(msg));
		// returns 1 if the packet was sent successfully
		return server->endPacket();
#else
		EthernetClient client;
		if (client.connect(controllerIP, controllerPort)) {
			client.write(msg, strlen(msg));
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

bool MyGatewayTransportEthernet::available()
{
	bool available = false;

#ifdef IP_ADDRESS_DHCP
	// renew IP address using DHCP
	renewIP();
#endif

#ifdef MY_USE_UDP
	int packet_size = server->parsePacket();

	if (server->available()) {
		server->read(inputBuffer, MAX_RECEIVE_LENGTH);
		available = true;
	}
#else
	// if an incoming client connects, there will be
	// bytes available to read via the client object
	EthernetClient client = server->available();

	if (client) {
		while (client.available()) {
			// read the bytes incoming from the client
			char inChar = client.read();

			if (inputPos < MAX_RECEIVE_LENGTH - 1) {
				// if newline then command is complete
				if (inChar == '\n') {
					inputBuffer[inputPos] = 0;
					available = true;
				} else {
					inputBuffer[inputPos] = inChar;
					inputPos++;
				}
			}
		}
	}
	inputPos = 0;
#endif

	if (available) {
		// Parse message and return parse result
		available = protocol.parse(msg, inputBuffer);
	}

	return available;
}

MyMessage& MyGatewayTransportEthernet::receive()
{
	// Return the last parsed message
	return msg;
}

void MyGatewayTransportEthernet::setControllerIPPort(const IPAddress& addr, const uint16_t port) {
	controllerIP = addr;

	/* set the port if some legal value passed */
	if (port > 0)
		controllerPort = port;
}

#ifdef IP_ADDRESS_DHCP
void MyGatewayTransportEthernet::renewIP()
{
	/* renew/rebind IP address
	 0 - nothing happened
	 1 - renew failed
	 2 - renew success
	 3 - rebinf failed
	 4 - rebind success
	 */
	// TODO: Use millis from MyHw ??
	static unsigned long next_time = millis() + ip_renewal_period;
	unsigned long now = millis();

	// http://playground.arduino.cc/Code/TimingRollover
	if ((long)(now - next_time) < 0)
		return;

	if (Ethernet.maintain() & ~(0x06)) {
		/* Error occured -> IP was not renewed */
		return;
	}

	next_time = now + ip_renewal_period;
}
#endif /* IP_ADDRESS_DHCP */
