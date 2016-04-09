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
	static bool clientsConnected[MY_GATEWAY_MAX_CLIENTS];
#endif

#if defined(MY_USE_UDP)
	EthernetUDP _ethernetServer;
#else
	EthernetServer _ethernetServer(_ethernetGatewayPort);
#endif


#if defined(MY_GATEWAY_ESP8266)
	static EthernetClient clients[MY_GATEWAY_MAX_CLIENTS];
	static inputBuffer inputString[MY_GATEWAY_MAX_CLIENTS];
#else
	static EthernetClient client = EthernetClient();
	static inputBuffer inputString;
#endif


#ifndef MY_IP_ADDRESS
	void gatewayTransportRenewIP();
#endif

// On W5100 boards with SPI_EN exposed we can use the real SPI bus together with radio
// (if we enable it during usage)
#ifdef MY_W5100_SPI_EN
	void _w5100_spi_en(boolean enable)
	{
	  if (enable)
	  {
	    // Pull up pin
	    pinMode(MY_W5100_SPI_EN, INPUT);
	    digitalWrite(MY_W5100_SPI_EN, HIGH);
	  }
	  else
	  {
	    // Ground pin
	    pinMode(MY_W5100_SPI_EN, OUTPUT);
	    digitalWrite(MY_W5100_SPI_EN, LOW);
	  }
	}
#else
	#define _w5100_spi_en(x)
#endif



bool gatewayTransportInit() {
	_w5100_spi_en(true);
	#if defined(MY_GATEWAY_ESP8266)
		(void)WiFi.begin(MY_ESP8266_SSID, MY_ESP8266_PASSWORD);
		#ifdef MY_IP_ADDRESS
			WiFi.config(_ethernetGatewayIP, gateway, subnet);
		#endif
		while (WiFi.status() != WL_CONNECTED)
		{
			delay(500);
			MY_SERIALDEVICE.print(".");
			yield();
		}
		MY_SERIALDEVICE.print(F("IP: "));
		MY_SERIALDEVICE.println(WiFi.localIP());

	#else
		#ifdef MY_IP_ADDRESS
			Ethernet.begin(_ethernetGatewayMAC, _ethernetGatewayIP);
			MY_SERIALDEVICE.print(F("IP: "));
			MY_SERIALDEVICE.println(Ethernet.localIP());
		#else
			// Get IP address from DHCP
			Ethernet.begin(_ethernetGatewayMAC);
			MY_SERIALDEVICE.print(F("IP: "));
			MY_SERIALDEVICE.println(Ethernet.localIP());
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
	#endif /* USE_UDP */
	_w5100_spi_en(false);
	return true;
}

bool gatewayTransportSend(MyMessage &message)
{
	bool ret = true;
	char *_ethernetMsg = protocolFormat(message);

	_w5100_spi_en(true);
	#if defined(MY_CONTROLLER_IP_ADDRESS)
		#if defined(MY_USE_UDP)
			_ethernetServer.beginPacket(_ethernetControllerIP, MY_PORT);
			_ethernetServer.write(_ethernetMsg, strlen(_ethernetMsg));
			// returns 1 if the packet was sent successfully
			ret = _ethernetServer.endPacket();
		#else
			EthernetClient client;
			#if defined(MY_CONTROLLER_URL_ADDRESS)
				if (client.connect(MY_CONTROLLER_URL_ADDRESS, MY_PORT)) {
			#else
				if (client.connect(_ethernetControllerIP, MY_PORT)) {
			#endif
				client.write(_ethernetMsg, strlen(_ethernetMsg));
				client.stop();
			}
			else {
				// connecting to the server failed!
				ret = false;
			}
		#endif
	#else
		// Send message to connected clients
		#if defined(MY_GATEWAY_ESP8266)
			for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++)
			{
				if (clients[i] && clients[i].connected())
				{
					clients[i].write((uint8_t*)_ethernetMsg, strlen(_ethernetMsg));
				}
			}
		#else
			_ethernetServer.write(_ethernetMsg);
		#endif
	#endif
	_w5100_spi_en(false);
	return ret;

}


#if defined(MY_GATEWAY_ESP8266)
	bool _readFromClient(uint8_t i) {
		while (clients[i].connected() && clients[i].available()) {
			char inChar = clients[i].read();
			if (inputString[i].idx < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
				// if newline then command is complete
				if (inChar == '\n' || inChar == '\r') {
					// Add string terminator and prepare for the next message
					inputString[i].string[inputString[i].idx] = 0;
					debug(PSTR("Client %d: %s\n"), i, inputString[i].string);
					inputString[i].idx = 0;
					if (protocolParse(_ethernetMsg, inputString[i].string)) {
						return true;
					}

				} else {
					// add it to the inputString:
					inputString[i].string[inputString[i].idx++] = inChar;
				}
			} else {
				// Incoming message too long. Throw away
				debug(PSTR("Client %d: Message too long\n"), i);
				inputString[i].idx = 0;
				// Finished with this client's message. Next loop() we'll see if there's more to read.
				break;
			}
		}
		return false;
	}
#else
	bool _readFromClient() {
		while (client.connected() && client.available()) {
			char inChar = client.read();
			if (inputString.idx < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
				// if newline then command is complete
				if (inChar == '\n' || inChar == '\r') {
					// Add string terminator and prepare for the next message
					inputString.string[inputString.idx] = 0;
					debug(PSTR("Eth: %s\n"), inputString.string);
					inputString.idx = 0;
					if (protocolParse(_ethernetMsg, inputString.string)) {
						return true;
					}

				} else {
					// add it to the inputString:
					inputString.string[inputString.idx++] = inChar;
				}
			} else {
				// Incoming message too long. Throw away
				debug(PSTR("Eth: Message too long\n"));
				inputString.idx = 0;
				// Finished with this client's message. Next loop() we'll see if there's more to read.
				break;
			}
		}
		return false;
	}
#endif


bool gatewayTransportAvailable()
{
	_w5100_spi_en(true);
	#if !defined(MY_IP_ADDRESS) && defined(MY_GATEWAY_W5100)
		// renew IP address using DHCP
		gatewayTransportRenewIP();
	#endif

	#ifdef MY_USE_UDP

		int packet_size = _ethernetServer.parsePacket();

		if (packet_size) {
			//debug(PSTR("UDP packet available. Size:%d\n"), packet_size);
			#if defined(MY_GATEWAY_ESP8266)
				_ethernetServer.read(inputString[0].string, MY_GATEWAY_MAX_RECEIVE_LENGTH);
				debug(PSTR("UDP packet received: %s\n"), inputString[0].string);
				return protocolParse(_ethernetMsg, inputString[0].string);
			#else
				_ethernetServer.read(inputString.string, MY_GATEWAY_MAX_RECEIVE_LENGTH);
				_w5100_spi_en(false);
				debug(PSTR("UDP packet received: %s\n"), inputString.string);
				return protocolParse(_ethernetMsg, inputString.string);
			#endif
		}
	#else
		#if defined(MY_GATEWAY_ESP8266)
			// ESP8266: Go over list of clients and stop any that are no longer connected.
			// If the server has a new client connection it will be assigned to a free slot.
			bool allSlotsOccupied = true;
			for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++) {
				if (!clients[i].connected()) {
					if (clientsConnected[i]) {
						debug(PSTR("Client %d disconnected\n"), i);
						clients[i].stop();
					}
					//check if there are any new clients
					if (_ethernetServer.hasClient()) {
						clients[i] = _ethernetServer.available();
						inputString[i].idx = 0;
						debug(PSTR("Client %d connected\n"), i);
						_w5100_spi_en(false);
						gatewayTransportSend(buildGw(_msg, I_GATEWAY_READY).set("Gateway startup complete."));
						_w5100_spi_en(true);
						if (presentation)
							presentation();
					}
				}
				bool connected = clients[i].connected();
				clientsConnected[i] = connected;
				allSlotsOccupied &= connected;
			}
			if (allSlotsOccupied && _ethernetServer.hasClient()) {
				//no free/disconnected spot so reject
				debug(PSTR("No free slot available\n"));
				EthernetClient c = _ethernetServer.available();
				c.stop();
			}
				// Loop over clients connect and read available data
			for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++) {
				if (_readFromClient(i)) {
					_w5100_spi_en(false);
					return true;
				}
			}
		#else
			// W5100/ENC module does not have hasClient-method. We can only serve one client at the time.
			EthernetClient newclient = _ethernetServer.available();
			// if a new client connects make sure to dispose any previous existing sockets
			if (newclient) {
				if (client != newclient) {
					client.stop();
					client = newclient;
					debug(PSTR("Eth: connect\n"));
					_w5100_spi_en(false);
					gatewayTransportSend(buildGw(_msg, I_GATEWAY_READY).set("Gateway startup complete."));
					if (presentation)
						presentation();
				}
			}
			if (client) {
				if (!client.connected()) {
					debug(PSTR("Eth: disconnect\n"));
					client.stop();
				} else {
					if (_readFromClient()) {
						_w5100_spi_en(false);
						return true;
					}
				}
			}
		#endif
	#endif
	_w5100_spi_en(false);
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
	static unsigned long next_time = hwMillis() + MY_IP_RENEWAL_INTERVAL;
	unsigned long now = hwMillis();

	// http://playground.arduino.cc/Code/TimingRollover
	if ((long)(now - next_time) < 0)
		return;
	if (Ethernet.maintain() & ~(0x06)) {
		debug(PSTR("IP was not renewed correctly\n"));
		/* Error occured -> IP was not renewed */
		return;
	}
	_w5100_spi_en(false);
	next_time = now + MY_IP_RENEWAL_INTERVAL;
}
#endif /* IP_ADDRESS_DHCP */
