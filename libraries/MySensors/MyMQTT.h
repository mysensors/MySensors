/*
The MySensors library adds a new layer on top of the RF24 library.
It handles radio network routing, relaying and ids.

Created by Henrik Ekblad <henrik.ekblad@gmail.com>
Modified by Daniel Wiegert
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.
*/

#ifndef MyMQTT_h
#define MyMQTT_h

#include "MySensor.h"


//////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define TCPDUMP					// Dump TCP packages.
#endif

#define MQTT_FIRST_SENSORID	20  		// If you want manually configured nodes below this value. 255 = Disable
#define MQTT_LAST_SENSORID	254 		// 254 is max! 255 reserved.
#define MQTT_BROKER_PREFIX	"MyMQTT"	// First prefix in MQTT tree, keep short!
#define MQTT_SEND_SUBSCRIPTION 1		// Send empty payload (request) to node upon MQTT client subscribe request.
// NOTE above : Beware to check if there is any length on payload in your incommingMessage code:
// Example: if (msg.type==V_LIGHT && strlen(msg.getString())>0) otherwise the code might do strange things.

//////////////////////////////////////////////////////////////////

#define EEPROM_LATEST_NODE_ADDRESS ((uint8_t)EEPROM_LOCAL_CONFIG_ADDRESS)
#define MQTT_MAX_PACKET_SIZE 100

#define MQTTPROTOCOLVERSION 3
#define MQTTCONNECT     1  // Client request to connect to Server
#define MQTTCONNACK     2  // Connect Acknowledgment
#define MQTTPUBLISH     3  // Publish message
#define MQTTPUBACK      4  // Publish Acknowledgment
#define MQTTPUBREC      5  // Publish Received (assured delivery part 1)
#define MQTTPUBREL      6  // Publish Release (assured delivery part 2)
#define MQTTPUBCOMP     7  // Publish Complete (assured delivery part 3)
#define MQTTSUBSCRIBE   8  // Client Subscribe request
#define MQTTSUBACK      9  // Subscribe Acknowledgment
#define MQTTUNSUBSCRIBE 10 // Client Unsubscribe request
#define MQTTUNSUBACK    11 // Unsubscribe Acknowledgment
#define MQTTPINGREQ     12 // PING Request
#define MQTTPINGRESP    13 // PING Response
#define MQTTDISCONNECT  14 // Client is Disconnecting
#define MQTTReserved    15 // Reserved

#define MQTTQOS0        (0 << 1)
#define MQTTQOS1        (1 << 1)
#define MQTTQOS2        (2 << 1)

class MyMQTT :
public MySensor {

public:
	MyMQTT(uint8_t _cepin=5, uint8_t _cspin=6);
	void begin(rf24_pa_dbm_e paLevel=RF24_PA_LEVEL_GW, uint8_t channel=RF24_CHANNEL, rf24_datarate_e dataRate=RF24_DATARATE, void (*dataCallback)
			(const char *, uint8_t *)=NULL, uint8_t _rx=6, uint8_t _tx=5, uint8_t _er=4 );
	void processRadioMessage();
	void processMQTTMessage(char *inputString, uint8_t inputPos);
private:
	void (*dataCallback)(const char *, uint8_t *);
	void SendMQTT(MyMessage &msg);
	void ledTimers();
	void rxBlink(uint8_t cnt);
	void txBlink(uint8_t cnt);
	void errBlink(uint8_t cnt);

	uint8_t MQTTClients;
	char buffer[MQTT_MAX_PACKET_SIZE];
	char convBuf[MAX_PAYLOAD*2+1];
	uint8_t buffsize;
	char *getType(char *b, const char **index);
};

extern void ledTimersInterrupt();

#endif
