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

#include "MyGatewayTransport.h"

#if defined(MY_CONTROLLER_IP_ADDRESS)
	IPAddress _ethernetControllerIP(MY_CONTROLLER_IP_ADDRESS);
#endif

#if defined(MY_IP_ADDRESS)
	IPAddress _ethernetGatewayIP(MY_IP_ADDRESS);
#endif
byte _ethernetGatewayMAC[] = { MY_MAC_ADDRESS };
uint16_t _ethernetGatewayPort = MY_PORT;
MyMessage _ethernetMsg;

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

// gatewayTransportSend(buildGw(_msg, I_GATEWAY_READY).set("Gateway startup complete."));

typedef struct
{
  char string[MY_GATEWAY_MAX_RECEIVE_LENGTH];
  uint8_t idx;
} inputBuffer;

#if defined(MY_GATEWAY_ESP8266)
	// Some re-defines to make code more readable below
	#define EthernetServer WiFiServer
	#define EthernetClient WiFiClient
	#define EthernetUDP WiFiUDP

	#if defined(MY_IP_ADDRESS)
		IPAddress gateway(MY_IP_GATEWAY_ADDRESS);
		IPAddress subnet(MY_IP_SUBNET_ADDRESS);
	#endif

#endif



#if defined(MY_USE_UDP)
	EthernetUDP _ethernetServer;
#else
	EthernetServer _ethernetServer(_ethernetGatewayPort);
	uint8_t _ethernetInputPos;
#endif /* USE_UDP */


static EthernetClient clients[MY_GATEWAY_MAX_CLIENTS];
static bool clientsConnected[MY_GATEWAY_MAX_CLIENTS];
static inputBuffer inputString[MY_GATEWAY_MAX_CLIENTS];


#ifndef MY_IP_ADDRESS
	void gatewayTransportRenewIP();
#endif


bool gatewayTransportInit() {
	#if defined(MY_GATEWAY_ESP8266)
		(void)WiFi.begin(MY_ESP8266_SSID, MY_ESP8266_PASSWORD);
		#ifdef MY_IP_ADDRESS
			WiFi.config(_ethernetGatewayIP, gateway, subnet);
		#endif
		while (WiFi.status() != WL_CONNECTED)
		{
			delay(500);
			Serial.print(".");
			yield();
		}
		Serial.println(PSTR("Connected to AP!"));
		Serial.print(PSTR("IP: "));
		Serial.println(WiFi.localIP());

	#else
		#ifdef MY_IP_ADDRESS
			Ethernet.begin(_ethernetGatewayMAC, _ethernetGatewayIP);
		#else
			// Get IP address from DHCP
			Ethernet.begin(_ethernetGatewayMAC);
		#endif /* IP_ADDRESS_DHCP */
		// give the Ethernet interface a second to initialize
		// TODO: use HW delay
		delay(1000);
	#endif

	#ifdef MY_USE_UDP
		_ethernetServer.begin(_ethernetGatewayPort);
	#else
		// we have to use pointers due to the constructor of EthernetServer
		_ethernetServer.begin();
		#if defined(MY_GATEWAY_ESP8266)
			_ethernetServer.setNoDelay(true);
		#endif
	#endif /* USE_UDP */
	return true;
}

bool gatewayTransportSend(MyMessage &message)
{
	char *_ethernetMsg = protocolFormat(message);

	#if defined(MY_CONTROLLER_IP_ADDRESS)
		#if defined(MY_USE_UDP)
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

		#endif
	#else
	  // Send to connected clients
	  for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++)
	  {
	    if (clients[i] && clients[i].connected())
	    {
		   clients[i].write((uint8_t*)_ethernetMsg, strlen(_ethernetMsg));
	    }
	  }
	  // TODO: What do we return here?
	  return true;
	#endif
}

bool gatewayTransportAvailable()
{
	bool available = false;

	#if !defined(MY_IP_ADDRESS) && !defined(MY_GATEWAY_ESP8266)
		// renew IP address using DHCP
		gatewayTransportRenewIP();
	#endif

	#ifdef MY_USE_UDP
		int packet_size = _ethernetServer.parsePacket();

		if (_ethernetServer.available()) {
			_ethernetServer.read(inputString[0].string, MY_GATEWAY_MAX_RECEIVE_LENGTH);
			return protocolParse(_ethernetMsg, inputString[0].string);
		}
	#else
		// Go over list of clients and stop any that are no longer connected.
		// If the server has a new client connection it will be assigned to a free slot.
		bool allSlotsOccupied = true;
		for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++) {
			if (!clients[i].connected()) {
				if (clientsConnected[i]) {
					Serial.print("Client ");
					Serial.print(i);
					Serial.println(" disconnected");
					clients[i].stop();
				}
				//check if there are any new clients
				if (_ethernetServer.hasClient()) {
					clients[i] = _ethernetServer.available();
					inputString[i].idx = 0;
					Serial.print("Client ");
					Serial.print(i);
					Serial.println(" connected");
					gatewayTransportSend(buildGw(_msg, I_GATEWAY_READY).set("Gateway startup complete."));
				}
			}
			bool connected = clients[i].connected();
			clientsConnected[i] = connected;
			allSlotsOccupied &= connected;
		}
		if (allSlotsOccupied && _ethernetServer.hasClient()) {
			//no free/disconnected spot so reject
			Serial.println("No free slot available");
			EthernetClient c = _ethernetServer.available();
			c.stop();
		}
			// Loop over clients connect and read available data
		for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++) {
			while (clients[i].connected() && clients[i].available()) {
				char inChar = clients[i].read();
				Serial.print(inChar);
				if (inputString[i].idx < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
					// if newline then command is complete
					if (inChar == '\n') {
						// a command was issued by the client
						// we will now try to send it to the actuator
						// echo the string to the serial port
						Serial.print(PSTR("Client "));
						Serial.print(i);
						Serial.print(": ");
						Serial.println(inputString[i].string);

						// clear the string:
						inputString[i].idx = 0;

						bool ret = protocolParse(_ethernetMsg, inputString[i].string);
						inputString[i].string[inputString[i].idx] = 0;
						if (ret)
							return ret;

					} else {
						// add it to the inputString:
						inputString[i].string[inputString[i].idx++] = inChar;
					}
				} else {
					// Incoming message too long. Throw away
					Serial.print(PSTR("Client "));
					Serial.print(i);
					Serial.println(PSTR(": Message too long"));
					inputString[i].idx = 0;
					// Finished with this client's message. Next loop() we'll see if there's more to read.
					break;
				}
			}
		}

	#endif
	return false;
}

MyMessage& gatewayTransportReceive()
{
	// Return the last parsed message
	return _ethernetMsg;
}


#if !defined(MY_IP_ADDRESS) && !defined(MY_GATEWAY_ESP8266)
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
