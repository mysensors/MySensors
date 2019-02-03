/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Tomas Hozza <thozza@gmail.com>
 * Copyright (C) 2015  Tomas Hozza
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyGatewayTransport.h"

// global variables
extern MyMessage _msgTmp;

// housekeeping, remove for 3.0.0
#ifdef MY_ESP8266_SSID
#warning MY_ESP8266_SSID is deprecated, use MY_WIFI_SSID instead!
#define MY_WIFI_SSID MY_ESP8266_SSID
#undef MY_ESP8266_SSID // cleanup
#endif

#ifdef MY_ESP8266_PASSWORD
#warning MY_ESP8266_PASSWORD is deprecated, use MY_WIFI_PASSWORD instead!
#define MY_WIFI_PASSWORD MY_ESP8266_PASSWORD
#undef MY_ESP8266_PASSWORD // cleanup
#endif

#ifdef MY_ESP8266_BSSID
#warning MY_ESP8266_BSSID is deprecated, use MY_WIFI_BSSID instead!
#define MY_WIFI_BSSID MY_ESP8266_BSSID
#undef MY_ESP8266_BSSID // cleanup
#endif

#ifdef MY_ESP8266_HOSTNAME
#warning MY_ESP8266_HOSTNAME is deprecated, use MY_HOSTNAME instead!
#define MY_HOSTNAME MY_ESP8266_HOSTNAME
#undef MY_ESP8266_HOSTNAME // cleanup
#endif

#if defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
#if !defined(MY_WIFI_SSID)
#error ESP8266/ESP32 gateway: MY_WIFI_SSID not defined!
#endif
#endif

#if defined(MY_CONTROLLER_IP_ADDRESS)
#define _ethernetControllerIP IPAddress(MY_CONTROLLER_IP_ADDRESS)
#endif

#if defined(MY_IP_ADDRESS)
#define _ethernetGatewayIP IPAddress(MY_IP_ADDRESS)
#if defined(MY_IP_GATEWAY_ADDRESS)
#define _gatewayIp IPAddress(MY_IP_GATEWAY_ADDRESS)
#elif defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
// Assume the gateway will be the machine on the same network as the local IP
// but with last octet being '1'
#define _gatewayIp IPAddress(_ethernetGatewayIP[0], _ethernetGatewayIP[1], _ethernetGatewayIP[2], 1)
#endif /* End of MY_IP_GATEWAY_ADDRESS */
#if defined(MY_IP_SUBNET_ADDRESS)
#define _subnetIp IPAddress(MY_IP_SUBNET_ADDRESS)
#elif defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
#define _subnetIp IPAddress(255, 255, 255, 0)
#endif /* End of MY_IP_SUBNET_ADDRESS */
#endif /* End of MY_IP_ADDRESS */

uint8_t _ethernetGatewayMAC[] = { MY_MAC_ADDRESS };
uint16_t _ethernetGatewayPort = MY_PORT;
MyMessage _ethernetMsg;

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))

typedef struct {
	// Suppress the warning about unused members in this struct because it is used through a complex
	// set of preprocessor directives
	// cppcheck-suppress unusedStructMember
	char string[MY_GATEWAY_MAX_RECEIVE_LENGTH];
	// cppcheck-suppress unusedStructMember
	uint8_t idx;
} inputBuffer;

#if defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
// Some re-defines to make code more readable below
#define EthernetServer WiFiServer
#define EthernetClient WiFiClient
#define EthernetUDP WiFiUDP
#endif

#if defined(MY_GATEWAY_CLIENT_MODE)
#if defined(MY_USE_UDP)
EthernetUDP _ethernetServer;
#endif /* End of MY_USE_UDP */
#elif defined(MY_GATEWAY_LINUX) /* Elif part of MY_GATEWAY_CLIENT_MODE */
EthernetServer _ethernetServer(_ethernetGatewayPort, MY_GATEWAY_MAX_CLIENTS);
#else /* Else part of MY_GATEWAY_CLIENT_MODE */
EthernetServer _ethernetServer(_ethernetGatewayPort);
#endif /* End of MY_GATEWAY_CLIENT_MODE */

#if defined(MY_GATEWAY_CLIENT_MODE)
static inputBuffer inputString;
#if defined(MY_USE_UDP)
// Nothing to do here
#else
static EthernetClient client = EthernetClient();
#endif /* End of MY_USE_UDP */
#elif defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32) || defined(MY_GATEWAY_LINUX)
static EthernetClient clients[MY_GATEWAY_MAX_CLIENTS];
static bool clientsConnected[MY_GATEWAY_MAX_CLIENTS];
static inputBuffer inputString[MY_GATEWAY_MAX_CLIENTS];
#else /* Else part of MY_GATEWAY_CLIENT_MODE */
static EthernetClient client = EthernetClient();
static inputBuffer inputString;
#endif /* End of MY_GATEWAY_CLIENT_MODE */

// On W5100 boards with SPI_EN exposed we can use the real SPI bus together with radio
// (if we enable it during usage)
void _w5100_spi_en(const bool enable)
{
#if defined(MY_W5100_SPI_EN)
	if (enable) {
		// Pull up pin
		hwPinMode(MY_W5100_SPI_EN, INPUT);
		hwDigitalWrite(MY_W5100_SPI_EN, HIGH);
	} else {
		// Ground pin
		hwPinMode(MY_W5100_SPI_EN, OUTPUT);
		hwDigitalWrite(MY_W5100_SPI_EN, LOW);
	}
#else
	(void)enable;
#endif
}

#if !defined(MY_IP_ADDRESS) && defined(MY_GATEWAY_W5100)
void gatewayTransportRenewIP(void)
{
	/* renew/rebind IP address
	 0 - nothing happened
	 1 - renew failed
	 2 - renew success
	 3 - rebind failed
	 4 - rebind success
	 */
	static uint32_t _nextIPRenewal = hwMillis() + MY_IP_RENEWAL_INTERVAL_MS;
	const uint32_t now = hwMillis();

	// http://playground.arduino.cc/Code/TimingRollover
	if ((int32_t)(now - _nextIPRenewal) < 0) {
		return;
	}
	if (Ethernet.maintain() & ~(0x06)) {
		GATEWAY_DEBUG(PSTR("!GWT:TRC:IP RENEW FAIL\n"));
		return;
	}
	_w5100_spi_en(false);
	_nextIPRenewal = now + MY_IP_RENEWAL_INTERVAL_MS;
}
#endif

bool gatewayTransportInit(void)
{
	_w5100_spi_en(true);

#if defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
	// Turn off access point
	WiFi.mode(WIFI_STA);
#if defined(MY_GATEWAY_ESP8266)
	WiFi.hostname(MY_HOSTNAME);
#elif defined(MY_GATEWAY_ESP32)
	WiFi.setHostname(MY_HOSTNAME);
#endif
#ifdef MY_IP_ADDRESS
	WiFi.config(_ethernetGatewayIP, _gatewayIp, _subnetIp);
#endif
	(void)WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD, 0, MY_WIFI_BSSID);
	while (WiFi.status() != WL_CONNECTED) {
		delay(1000);
		GATEWAY_DEBUG(PSTR("GWT:TIN:CONNECTING...\n"));
	}
	GATEWAY_DEBUG(PSTR("GWT:TIN:IP: %s\n"), WiFi.localIP().toString().c_str());
#elif defined(MY_GATEWAY_LINUX)
	// Nothing to do here
#else
#if defined(MY_IP_GATEWAY_ADDRESS) && defined(MY_IP_SUBNET_ADDRESS)
	// DNS server set to gateway ip
	Ethernet.begin(_ethernetGatewayMAC, _ethernetGatewayIP, _gatewayIp, _gatewayIp, _subnetIp);
#elif defined(MY_IP_ADDRESS)
	Ethernet.begin(_ethernetGatewayMAC, _ethernetGatewayIP);
#else /* Else part of MY_IP_GATEWAY_ADDRESS && MY_IP_SUBNET_ADDRESS */
	// Get IP address from DHCP
	if (!Ethernet.begin(_ethernetGatewayMAC)) {
		GATEWAY_DEBUG(PSTR("!GWT:TIN:DHCP FAIL\n"));
		_w5100_spi_en(false);
		return false;
	}
#endif /* End of MY_IP_GATEWAY_ADDRESS && MY_IP_SUBNET_ADDRESS */
	GATEWAY_DEBUG(PSTR("GWT:TIN:IP=%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 "\n"),
	              Ethernet.localIP()[0],
	              Ethernet.localIP()[1], Ethernet.localIP()[2], Ethernet.localIP()[3]);
	// give the Ethernet interface a second to initialize
	delay(1000);
#endif /* MY_GATEWAY_ESP8266 / MY_GATEWAY_ESP32 */

#if defined(MY_GATEWAY_CLIENT_MODE)
#if defined(MY_USE_UDP)
	_ethernetServer.begin(_ethernetGatewayPort);
#else /* Else part of MY_USE_UDP */
#if defined(MY_GATEWAY_LINUX) && defined(MY_IP_ADDRESS)
	client.bind(_ethernetGatewayIP);
#endif /* End of MY_GATEWAY_LINUX && MY_IP_ADDRESS */
#if defined(MY_CONTROLLER_URL_ADDRESS)
	if (client.connect(MY_CONTROLLER_URL_ADDRESS, MY_PORT)) {
#else
	if (client.connect(_ethernetControllerIP, MY_PORT)) {
#endif /* End of MY_CONTROLLER_URL_ADDRESS */
		GATEWAY_DEBUG(PSTR("GWT:TIN:ETH OK\n"));
		_w5100_spi_en(false);
		gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(F(MSG_GW_STARTUP_COMPLETE)));
		_w5100_spi_en(true);
		presentNode();
	} else {
		client.stop();
		GATEWAY_DEBUG(PSTR("!GWT:TIN:ETH FAIL\n"));
	}
#endif /* End of MY_USE_UDP */
#else /* Else part of MY_GATEWAY_CLIENT_MODE */
#if defined(MY_GATEWAY_LINUX) && defined(MY_IP_ADDRESS)
	_ethernetServer.begin(_ethernetGatewayIP);
#else
	// we have to use pointers due to the constructor of EthernetServer
	_ethernetServer.begin();
#endif /* End of MY_GATEWAY_LINUX && MY_IP_ADDRESS */
#endif /* End of MY_GATEWAY_CLIENT_MODE */

	_w5100_spi_en(false);
	return true;
}

bool gatewayTransportSend(MyMessage &message)
{
	int nbytes = 0;
	char *_ethernetMsg = protocolMyMessage2Serial(message);

	setIndication(INDICATION_GW_TX);

	_w5100_spi_en(true);
#if defined(MY_GATEWAY_CLIENT_MODE)
#if defined(MY_USE_UDP)
#if defined(MY_CONTROLLER_URL_ADDRESS)
	_ethernetServer.beginPacket(MY_CONTROLLER_URL_ADDRESS, MY_PORT);
#else
	_ethernetServer.beginPacket(_ethernetControllerIP, MY_PORT);
#endif /* End of MY_CONTROLLER_URL_ADDRESS */
	_ethernetServer.write((uint8_t *)_ethernetMsg, strlen(_ethernetMsg));
	// returns 1 if the packet was sent successfully
	nbytes = _ethernetServer.endPacket();
#else /* Else part of MY_USE_UDP */
	if (!client.connected()) {
		client.stop();
#if defined(MY_CONTROLLER_URL_ADDRESS)
		if (client.connect(MY_CONTROLLER_URL_ADDRESS, MY_PORT)) {
#else
		if (client.connect(_ethernetControllerIP, MY_PORT)) {
#endif /* End of MY_CONTROLLER_URL_ADDRESS */
			GATEWAY_DEBUG(PSTR("GWT:TPS:ETH OK\n"));
			_w5100_spi_en(false);
			gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
			_w5100_spi_en(true);
			presentNode();
		} else {
			// connecting to the server failed!
			GATEWAY_DEBUG(PSTR("!GWT:TPS:ETH FAIL\n"));
			_w5100_spi_en(false);
			return false;
		}
	}
	nbytes = client.write((const uint8_t*)_ethernetMsg, strlen(_ethernetMsg));
#endif /* End of MY_USE_UDP */
#else /* Else part of MY_GATEWAY_CLIENT_MODE */
	// Send message to connected clients
#if defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32)
	for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++) {
		if (clients[i] && clients[i].connected()) {
			nbytes += clients[i].write((uint8_t *)_ethernetMsg, strlen(_ethernetMsg));
		}
	}
#else /* Else part of MY_GATEWAY_ESPxx*/
	nbytes = _ethernetServer.write(_ethernetMsg);
#endif /* End of MY_GATEWAY_ESPxx */
#endif /* End of MY_GATEWAY_CLIENT_MODE */
	_w5100_spi_en(false);
	return (nbytes > 0);
}

#if defined(MY_USE_UDP)
// Nothing to do here
#else
#if (defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32) || defined(MY_GATEWAY_LINUX)) && !defined(MY_GATEWAY_CLIENT_MODE)
bool _readFromClient(uint8_t i)
{
	while (clients[i].connected() && clients[i].available()) {
		const char inChar = clients[i].read();
		if (inputString[i].idx < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
			// if newline then command is complete
			if (inChar == '\n' || inChar == '\r') {
				// Add string terminator and prepare for the next message
				inputString[i].string[inputString[i].idx] = 0;
				GATEWAY_DEBUG(PSTR("GWT:RFC:C=%" PRIu8 ",MSG=%s\n"), i, inputString[i].string);
				inputString[i].idx = 0;
				if (protocolSerial2MyMessage(_ethernetMsg, inputString[i].string)) {
					return true;
				}
			} else {
				// add it to the inputString:
				inputString[i].string[inputString[i].idx++] = inChar;
			}
		} else {
			// Incoming message too long. Throw away
			GATEWAY_DEBUG(PSTR("!GWT:RFC:C=%" PRIu8 ",MSG TOO LONG\n"), i);
			inputString[i].idx = 0;
			// Finished with this client's message. Next loop() we'll see if there's more to read.
			break;
		}
	}
	return false;
}
#else /* Else part of MY_GATEWAY_ESP8266 || MY_GATEWAY_LINUX || !MY_GATEWAY_CLIENT_MODE */
bool _readFromClient(void)
{
	while (client.connected() && client.available()) {
		const char inChar = client.read();
		if (inputString.idx < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
			// if newline then command is complete
			if (inChar == '\n' || inChar == '\r') {
				// Add string terminator and prepare for the next message
				inputString.string[inputString.idx] = 0;
				GATEWAY_DEBUG(PSTR("GWT:RFC:MSG=%s\n"), inputString.string);
				inputString.idx = 0;
				if (protocolSerial2MyMessage(_ethernetMsg, inputString.string)) {
					return true;
				}

			} else {
				// add it to the inputString:
				inputString.string[inputString.idx++] = inChar;
			}
		} else {
			// Incoming message too long. Throw away
			GATEWAY_DEBUG(PSTR("!GWT:RFC:MSG TOO LONG\n"));
			inputString.idx = 0;
			// Finished with this client's message. Next loop() we'll see if there's more to read.
			break;
		}
	}
	return false;
}
#endif /* End of MY_GATEWAY_ESP8266 || MY_GATEWAY_LINUX || !MY_GATEWAY_CLIENT_MODE */
#endif /* End of MY_USE_UDP */

bool gatewayTransportAvailable(void)
{
	_w5100_spi_en(true);
#if !defined(MY_IP_ADDRESS) && defined(MY_GATEWAY_W5100)
	// renew IP address using DHCP
	gatewayTransportRenewIP();
#endif

#if defined(MY_GATEWAY_CLIENT_MODE)
#if defined(MY_USE_UDP)
	int packet_size = _ethernetServer.parsePacket();

	if (packet_size) {
		_ethernetServer.read(inputString.string, MY_GATEWAY_MAX_RECEIVE_LENGTH);
		inputString.string[packet_size] = 0;
		GATEWAY_DEBUG(PSTR("GWT:TSA:UDP MSG=%s\n"), inputString.string);
		_w5100_spi_en(false);
		const bool ok = protocolSerial2MyMessage(_ethernetMsg, inputString.string);
		if (ok) {
			setIndication(INDICATION_GW_RX);
		}
		return ok;
	}
#else /* Else part of MY_USE_UDP */
	if (!client.connected()) {
		client.stop();
#if defined(MY_CONTROLLER_URL_ADDRESS)
		if (client.connect(MY_CONTROLLER_URL_ADDRESS, MY_PORT)) {
#else
		if (client.connect(_ethernetControllerIP, MY_PORT)) {
#endif /* End of MY_CONTROLLER_URL_ADDRESS */
			GATEWAY_DEBUG(PSTR("GWT:TSA:ETH OK\n"));
			_w5100_spi_en(false);
			gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(F(MSG_GW_STARTUP_COMPLETE)));
			_w5100_spi_en(true);
			presentNode();
		} else {
			GATEWAY_DEBUG(PSTR("!GWT:TSA:ETH FAIL\n"));
			_w5100_spi_en(false);
			return false;
		}
	}
	if (_readFromClient()) {
		setIndication(INDICATION_GW_RX);
		_w5100_spi_en(false);
		return true;
	}
#endif /* End of MY_USE_UDP */
#else /* Else part of MY_GATEWAY_CLIENT_MODE */
#if defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_ESP32) || defined(MY_GATEWAY_LINUX)
	// ESP8266/ESP32: Go over list of clients and stop any that are no longer connected.
	// If the server has a new client connection it will be assigned to a free slot.
	bool allSlotsOccupied = true;
	for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++) {
		if (!clients[i].connected()) {
			if (clientsConnected[i]) {
				GATEWAY_DEBUG(PSTR("GWT:TSA:C=%" PRIu8 ",DISCONNECTED\n"), i);
				clients[i].stop();
			}
			//check if there are any new clients
			if (_ethernetServer.hasClient()) {
				clients[i] = _ethernetServer.available();
				inputString[i].idx = 0;
				GATEWAY_DEBUG(PSTR("GWT:TSA:C=%" PRIu8 ",CONNECTED\n"), i);
				gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
				// Send presentation of locally attached sensors (and node if applicable)
				presentNode();
			}
		}
		bool connected = clients[i].connected();
		clientsConnected[i] = connected;
		allSlotsOccupied &= connected;
	}
	if (allSlotsOccupied && _ethernetServer.hasClient()) {
		//no free/disconnected spot so reject
		GATEWAY_DEBUG(PSTR("!GWT:TSA:NO FREE SLOT\n"));
		EthernetClient c = _ethernetServer.available();
		c.stop();
	}
	// Loop over clients connect and read available data
	for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++) {
		if (_readFromClient(i)) {
			setIndication(INDICATION_GW_RX);
			_w5100_spi_en(false);
			return true;
		}
	}
#else /* Else part of MY_GATEWAY_ESP8266 || MY_GATEWAY_LINUX */
	// W5100/ENC module does not have hasClient-method. We can only serve one client at the time.
	EthernetClient newclient = _ethernetServer.available();
	// if a new client connects make sure to dispose any previous existing sockets
	if (newclient) {
		if (client != newclient) {
			client.stop();
			client = newclient;
			GATEWAY_DEBUG(PSTR("GWT:TSA:ETH OK\n"));
			_w5100_spi_en(false);
			gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set(MSG_GW_STARTUP_COMPLETE));
			_w5100_spi_en(true);
			presentNode();
		}
	}
	if (client) {
		if (!client.connected()) {
			GATEWAY_DEBUG(PSTR("!GWT:TSA:ETH FAIL\n"));
			client.stop();
		} else {
			if (_readFromClient()) {
				setIndication(INDICATION_GW_RX);
				_w5100_spi_en(false);
				return true;
			}
		}
	}
#endif /* End of MY_GATEWAY_ESP8266 || MY_GATEWAY_LINUX */
#endif /* End of MY_GATEWAY_CLIENT_MODE */
	_w5100_spi_en(false);
	return false;
}

MyMessage& gatewayTransportReceive(void)
{
	// Return the last parsed message
	return _ethernetMsg;
}


