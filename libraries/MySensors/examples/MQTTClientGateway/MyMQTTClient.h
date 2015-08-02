/*
The MySensors library adds a new layer on top of the RF24 library.
It handles radio network routing, relaying and ids.

Created by Henrik Ekblad <henrik.ekblad@gmail.com>
Modified by Daniel Wiegert
Modified by Norbert Truchsess <norbert.truchsess@t-online.de>
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.
*/


#ifndef MyMQTTClient_h
#define MyMQTTClient_h

#include "MySensor.h"
#include "PubSubClient.h"


#define DSRTC // RTC Dallas support -> disable debug and use isp programmer to free memory


//////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define TCPDUMP					// Dump TCP packages.
#endif

#define MQTT_FIRST_SENSORID	20  		// If you want manually configured nodes below this value. 255 = Disable
#define MQTT_LAST_SENSORID	254 		// 254 is max! 255 reserved.
#define MQTT_PREFIX	"MyMQTT"	        // First prefix in MQTT tree, keep short!
#define MQTT_SEND_SUBSCRIPTION 1		// Send empty payload (request) to node upon MQTT client subscribe request.

// NOTE above : Beware to check if there is any length on payload in your incommingMessage code:
// Example: if (msg.type==V_LIGHT && strlen(msg.getString())>0) otherwise the code might do strange things.

//////////////////////////////////////////////////////////////////

#define EEPROM_LATEST_NODE_ADDRESS ((uint8_t)EEPROM_LOCAL_CONFIG_ADDRESS)

class MyMQTTClient :
public MySensor {

public:
	MyMQTTClient(PubSubClient client, uint8_t _cepin=5, uint8_t _cspin=6);
	void begin(rf24_pa_dbm_e paLevel=RF24_PA_LEVEL_GW, uint8_t channel=RF24_CHANNEL, rf24_datarate_e dataRate=RF24_DATARATE, uint8_t _rx=6, uint8_t _tx=5, uint8_t _er=4 );
	void processRadioMessage();
	void processMQTTMessage(char* topic, byte* payload, unsigned int length);
private:
	void SendMQTT(MyMessage &msg);
	void ledTimers();
	void rxBlink(uint8_t cnt);
	void txBlink(uint8_t cnt);
	void errBlink(uint8_t cnt);

	PubSubClient client;
	char buffer[MQTT_MAX_PACKET_SIZE];
	char convBuf[MAX_PAYLOAD*2+1];
	uint8_t buffsize;
	char *getType(char *b, const char **index);
};

extern void ledTimersInterrupt();

#endif
