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

// Topic structure: MY_MQTT_TOPIC_PREFIX/NODE-ID/SENSOR-ID/CMD-TYPE/ACK-FLAG/SUB-TYPE



IPAddress _brokerIp(MY_CONTROLLER_IP_ADDRESS);

#if defined(MY_GATEWAY_ESP8266)
	#define EthernetClient WiFiClient
	IPAddress _gatewayIp(MY_IP_GATEWAY_ADDRESS);
	IPAddress _subnetIp(MY_IP_SUBNET_ADDRESS);
#else
	byte _clientMAC[] = { MY_MAC_ADDRESS };
#endif

#if defined(MY_IP_ADDRESS)
	IPAddress _clientIp(MY_IP_ADDRESS);
#endif

EthernetClient _ethClient;
PubSubClient _client(_ethClient);
bool _available = false;
char _convBuffer[MAX_PAYLOAD*2+1];
char _fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH];
MyMessage _mqttMsg;



bool gatewayTransportSend(MyMessage &message) {
	if (!_client.connected())
		return false;
	snprintf_P(_fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH, PSTR(MY_MQTT_TOPIC_PREFIX "/%d/%d/%d/%d/%d"), message.sender, message.sensor, mGetCommand(message), mGetAck(message), message.type);
	debug(PSTR("Sending message on topic: %s\n"), _fmtBuffer);
	return _client.publish(_fmtBuffer, message.getString(_convBuffer));
}



void incomingMQTT(char* topic, byte* payload,
                        unsigned int length)
{
	debug(PSTR("Message arrived on topic: %s\n"), topic);
	char *str, *p;
	uint8_t i = 0;
	for (str = strtok_r(topic, "/", &p); str && i <= 5;
			str = strtok_r(NULL, "/", &p)) {
		switch (i) {
			case 0: {
				// Topic prefix
				if (strcmp_P(str, MY_MQTT_TOPIC_PREFIX) != 0) {
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
				mSetCommand(_mqttMsg, atoi(str));
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
				char* ca;
				ca = (char *) payload;
				ca += length;
				*ca = '\0';
				_mqttMsg.set((const char*) payload);
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
		_client.subscribe(MY_MQTT_TOPIC_PREFIX "/+/+/+/+/+");
		return true;
	}
	return false;
}

bool gatewayTransportInit() {
	_client.setServer(_brokerIp, MY_PORT);
	_client.setCallback(incomingMQTT);

  	#if defined(MY_GATEWAY_ESP8266)
		(void)WiFi.begin(MY_ESP8266_SSID, MY_ESP8266_PASSWORD);
		#ifdef MY_IP_ADDRESS
			WiFi.config(_clientIp, _gatewayIp, _subnetIp);
		#endif
		while (WiFi.status() != WL_CONNECTED)
		{
			delay(500);
			Serial.print(".");
			yield();
		}
		Serial.print(F("IP: "));
		Serial.println(WiFi.localIP());
	#else
		#ifdef MY_IP_ADDRESS
			Ethernet.begin(_clientMAC, _clientIp);
			Serial.print(F("IP: "));
			Serial.println(Ethernet.localIP());
		#else
			// Get IP address from DHCP
			Ethernet.begin(_clientMAC);
			Serial.print(F("IP: "));
			Serial.println(Ethernet.localIP());
		#endif /* IP_ADDRESS_DHCP */
		// give the Ethernet interface a second to initialize
		// TODO: use HW delay
		wait(1000);
	#endif
	return true;
}


bool gatewayTransportAvailable() {
	if (!_client.connected()) {
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
