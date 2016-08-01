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
*/


// Topic structure: MY_MQTT_PUBLISH_TOPIC_PREFIX/NODE-ID/SENSOR-ID/CMD-TYPE/ACK-FLAG/SUB-TYPE

#include "MyGatewayTransport.h"

#if defined MY_CONTROLLER_IP_ADDRESS
	IPAddress _brokerIp(MY_CONTROLLER_IP_ADDRESS);
#endif

#if defined(MY_GATEWAY_ESP8266)
	#define EthernetClient WiFiClient
	#if defined(MY_IP_ADDRESS)
		IPAddress _gatewayIp(MY_IP_GATEWAY_ADDRESS);
		IPAddress _subnetIp(MY_IP_SUBNET_ADDRESS);
	#endif
#else
	byte _MQTT_clientMAC[] = { MY_MAC_ADDRESS };
#endif

#if defined(MY_IP_ADDRESS)
	IPAddress _MQTT_clientIp(MY_IP_ADDRESS);
#endif

static EthernetClient _MQTT_ethClient;
static PubSubClient _MQTT_client(_MQTT_ethClient);
static bool _MQTT_connecting = true;
static bool _MQTT_available = false;
static MyMessage _MQTT_msg;

uint8_t protocolH2i(char c) {
	uint8_t i = 0;
	if (c <= '9')
		i += c - '0';
	else if (c >= 'a')
		i += c - 'a' + 10;
	else
		i += c - 'A' + 10;
	return i;
}


bool gatewayTransportSend(MyMessage &message) {
	if (!_MQTT_client.connected())
		return false;
	setIndication(INDICATION_GW_TX);
	char _fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH];
	char _convBuffer[MAX_PAYLOAD * 2 + 1];
	snprintf_P(_fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH, PSTR(MY_MQTT_PUBLISH_TOPIC_PREFIX "/%d/%d/%d/%d/%d"), message.sender, message.sensor, mGetCommand(message), mGetAck(message), message.type);
	debug(PSTR("Sending message on topic: %s\n"), _fmtBuffer);
	return _MQTT_client.publish(_fmtBuffer, message.getString(_convBuffer));
}

void incomingMQTT(char* topic, byte* payload, unsigned int length) {
	debug(PSTR("Message arrived on topic: %s\n"), topic);
	char *str, *p;
	uint8_t i = 0;
	uint8_t bvalue[MAX_PAYLOAD];
	uint8_t blen = 0;
	uint8_t command = 0;
	for (str = strtok_r(topic, "/", &p); str && i <= 5;
		str = strtok_r(NULL, "/", &p)) {
		switch (i) {
		case 0: {
			// Topic prefix
			if (strcmp(str, MY_MQTT_SUBSCRIBE_TOPIC_PREFIX) != 0) {
				// Message not for us or malformed!
				return;
			}
			break;
		}
		case 1: {
			// Node id
			_MQTT_msg.destination = atoi(str);
			break;
		}
		case 2: {
			// Sensor id
			_MQTT_msg.sensor = atoi(str);
			break;
		}
		case 3: {
			// Command type
			command = atoi(str);
			mSetCommand(_MQTT_msg, command);
			break;
		}
		case 4: {
			// Ack flag
			mSetRequestAck(_MQTT_msg, atoi(str) ? 1 : 0);
			break;
		}
		case 5: {
			// Sub type
			_MQTT_msg.type = atoi(str);
			// Add payload
			if (command == C_STREAM) {
				blen = 0;
				uint8_t val;
				while (*payload) {
					val = protocolH2i(*payload++) << 4;
					val += protocolH2i(*payload++);
					bvalue[blen] = val;
					blen++;
				}
				_MQTT_msg.set(bvalue, blen);
			}
			else {
				char* ca;
				ca = (char *)payload;
				ca += length;
				*ca = '\0';
				_MQTT_msg.set((const char*)payload);
			}
			_MQTT_available = true;
		}
		}
		i++;
	}
}


bool reconnectMQTT() {
	debug(PSTR("Attempting MQTT connection...\n"));
	// Attempt to connect
	if (_MQTT_client.connect(MY_MQTT_CLIENT_ID
#if defined(MY_MQTT_USER) && defined(MY_MQTT_PASSWORD)
		, MY_MQTT_USER, MY_MQTT_PASSWORD
#endif
	)) {
		debug(PSTR("MQTT connected\n"));

		// Send presentation of locally attached sensors (and node if applicable)
		presentNode();

		// Once connected, publish an announcement...
		//_MQTT_client.publish("outTopic","hello world");
		// ... and resubscribe
		_MQTT_client.subscribe(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "/+/+/+/+/+");
		return true;
	}
	return false;
}

bool gatewayTransportConnect() {
	#if defined(MY_GATEWAY_ESP8266)
		while (WiFi.status() != WL_CONNECTED) {
			delay(500); // delay calls yield
			MY_SERIALDEVICE.print(".");
		}
		MY_SERIALDEVICE.print("IP: ");
		MY_SERIALDEVICE.println(WiFi.localIP());
	#else
		#ifdef MY_IP_ADDRESS
			Ethernet.begin(_MQTT_clientMAC, _MQTT_clientIp);
		#else
			// Get IP address from DHCP
			if (!Ethernet.begin(_MQTT_clientMAC)) {
				MY_SERIALDEVICE.print("DHCP FAILURE...");
				_MQTT_connecting = false;
				return false;
			}
		#endif
		MY_SERIALDEVICE.print("IP: ");
		MY_SERIALDEVICE.println(Ethernet.localIP());
		// give the Ethernet interface a second to initialize
		delay(1000);
	#endif
	return true;
}

bool gatewayTransportInit() {
	_MQTT_connecting = true;
	#if defined(MY_CONTROLLER_IP_ADDRESS)
		_MQTT_client.setServer(_brokerIp, MY_PORT);
	#else
		_MQTT_client.setServer(MY_CONTROLLER_URL_ADDRESS, MY_PORT);
	#endif

		_MQTT_client.setCallback(incomingMQTT);

	#if defined(MY_GATEWAY_ESP8266)
		// Turn off access point
		WiFi.mode(WIFI_STA);
		#if defined(MY_ESP8266_HOSTNAME)
			WiFi.hostname(MY_ESP8266_HOSTNAME);
		#endif
		(void)WiFi.begin(MY_ESP8266_SSID, MY_ESP8266_PASSWORD);
		#ifdef MY_IP_ADDRESS
			WiFi.config(_MQTT_clientIp, _gatewayIp, _subnetIp);
		#endif	
	#endif

	gatewayTransportConnect();

	_MQTT_connecting = false;
	return true;
}



bool gatewayTransportAvailable() {

	if (_MQTT_connecting)
		return false;

	//keep lease on dhcp address
	//Ethernet.maintain();
	if (!_MQTT_client.connected()) {
		//reinitialise client
		if (gatewayTransportConnect())
			reconnectMQTT();
		return false;
	}
	_MQTT_client.loop();
	return _MQTT_available;
}

MyMessage & gatewayTransportReceive() {
	// Return the last parsed message
	_MQTT_available = false;
	return _MQTT_msg;
}



