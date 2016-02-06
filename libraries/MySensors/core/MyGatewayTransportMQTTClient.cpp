/**
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


#include "MyConfig.h"
#include "MyProtocol.h"
#include "MyGatewayTransport.h"
#include "MyMessage.h"

// Topic structure: MY_MQTT_PUBLISH_TOPIC_PREFIX/NODE-ID/SENSOR-ID/CMD-TYPE/ACK-FLAG/SUB-TYPE

uint8_t protocolH2i(char c);


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
	byte _clientMAC[] = { MY_MAC_ADDRESS };
#endif

#if defined(MY_IP_ADDRESS)
	IPAddress _clientIp(MY_IP_ADDRESS);
#endif

EthernetClient _ethClient;
PubSubClient _client(_ethClient);
bool _connecting = true;
bool _available = false;
char _convBuffer[MAX_PAYLOAD*2+1];
char _fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH];
MyMessage _mqttMsg;



bool gatewayTransportSend(MyMessage &message) {
	if (!_client.connected())
		return false;
	snprintf_P(_fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH, PSTR(MY_MQTT_PUBLISH_TOPIC_PREFIX "/%d/%d/%d/%d/%d"), message.sender, message.sensor, mGetCommand(message), mGetAck(message), message.type);
	debug(PSTR("Sending message on topic: %s\n"), _fmtBuffer);
	return _client.publish(_fmtBuffer, message.getString(_convBuffer));
}



void incomingMQTT(char* topic, byte* payload,
                        unsigned int length)
{
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
				_mqttMsg.destination = atoi(str);
				break;
			}
			case 2: {
				// Sensor id
				_mqttMsg.sensor = atoi(str);
				break;
			}
			case 3: {
				// Command type
				command = atoi(str);
				mSetCommand(_mqttMsg, command);
				break;
			}
			case 4: {
				// Ack flag
				mSetRequestAck(_mqttMsg, atoi(str)?1:0);
				break;
			}
			case 5: {
				// Sub type
				_mqttMsg.type = atoi(str);
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
					_mqttMsg.set(bvalue, blen);
				} else {
					char* ca;
					ca = (char *) payload;
					ca += length;
					*ca = '\0';
					_mqttMsg.set((const char*) payload);
				}
				_available = true;
			}
		}
		i++;
	}
}


bool reconnectMQTT() {
	debug(PSTR("Attempting MQTT connection...\n"));
	// Attempt to connect
	if (_client.connect(MY_MQTT_CLIENT_ID
		#if defined(MY_MQTT_USER) && defined(MY_MQTT_PASSWORD)
			, MY_MQTT_USER, MY_MQTT_PASSWORD
		#endif
	)) {
		debug(PSTR("MQTT connected\n"));
		// Once connected, publish an announcement...
		//_client.publish("outTopic","hello world");
		// ... and resubscribe
		_client.subscribe(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "/+/+/+/+/+");
		return true;
	}
	return false;
}

bool gatewayTransportInit() {
	_connecting = true;
	#if defined(MY_CONTROLLER_IP_ADDRESS)
		_client.setServer(_brokerIp, MY_PORT);
	#else
		_client.setServer(MY_CONTROLLER_URL_ADDRESS, MY_PORT);
	#endif

	_client.setCallback(incomingMQTT);

  	#if defined(MY_GATEWAY_ESP8266)
		// Turn off access point
		WiFi.mode (WIFI_STA);
		(void)WiFi.begin(MY_ESP8266_SSID, MY_ESP8266_PASSWORD);
		#ifdef MY_IP_ADDRESS
			WiFi.config(_clientIp, _gatewayIp, _subnetIp);
		#endif
		while (WiFi.status() != WL_CONNECTED)
		{
			delay(500);
			MY_SERIALDEVICE.print(".");
			yield();
		}
		MY_SERIALDEVICE.print("IP: ");
		MY_SERIALDEVICE.println(WiFi.localIP());
	#else
		#ifdef MY_IP_ADDRESS
			Ethernet.begin(_clientMAC, _clientIp);
		#else
			// Get IP address from DHCP
			if (!Ethernet.begin(_clientMAC))
			{
				MY_SERIALDEVICE.print("DHCP FAILURE...");
				_connecting = false;
				return false;
			}
			MY_SERIALDEVICE.print("IP: ");
			MY_SERIALDEVICE.println(Ethernet.localIP());
		#endif

		// give the Ethernet interface a second to initialize
		// TODO: use HW delay
		wait(1000);
	#endif
	_connecting = false;
	return true;
}


bool gatewayTransportAvailable() {
	if (_connecting)
		return false;

	//keep lease on dhcp address
	//Ethernet.maintain();
	if (!_client.connected()) {
		//reinitialise client
		if (gatewayTransportInit())
			reconnectMQTT();
		return false;
	}
	_client.loop();
	return _available;
}

MyMessage & gatewayTransportReceive() {
	// Return the last parsed message
	_available = false;
	return _mqttMsg;
}


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
