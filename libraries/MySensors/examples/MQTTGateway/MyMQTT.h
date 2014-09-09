/*
The MySensors library adds a new layer on top of the RF24 library.
It handles radio network routing, relaying and ids.

Created by Henrik Ekblad <henrik.ekblad@gmail.com>
Modified by Daniel Wiegert
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.
*/

#ifdef DEBUG
#define TCPDUMP					// Dump TCP packages
#endif

#define MQTT_FIRST_SENSORID	20  		// If you want manually configured nodes below this value. 255 = Disable
#define MQTT_LAST_SENSORID	254 		// 254 is max! 255 reserved.
#define MQTT_BROKER_PREFIX	"MyMQTT"	// First prefix in MQTT tree, keep short!
#define MQTT_SEND_SUBSCRIPTION 1		// Send empty payload (request) to node upon MQTT client subscribe request.
// NOTE above : Beware to check if there is any length on payload in your incommingMessage code:
// Example: if (msg.type==V_LIGHT && strlen(msg.getString())>0) otherwise the code might do strange things.
#define MQTT_UNIT		"M"		// Select M for metric or I for imperial.
#define MQTT_TRANSLATE_TYPES			// V_TYPE in address, Comment if you want all numbers (MyMQTT/01/01/01)
 
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

#ifdef MQTT_TRANSLATE_TYPES

char V_0[] PROGMEM = "TEMP";		//V_TEMP
char V_1[] PROGMEM = "HUM";		//V_HUM
char V_2[] PROGMEM = "LIGHT";		//V_LIGHT
char V_3[] PROGMEM = "DIMMER";		//V_DIMMER
char V_4[] PROGMEM = "PRESSURE";	//V_PRESSURE
char V_5[] PROGMEM = "FORECAST";	//V_FORECAST
char V_6[] PROGMEM = "RAIN";		//V_RAIN
char V_7[] PROGMEM = "RAINRATE";	//V_RAINRATE
char V_8[] PROGMEM = "WIND";		//V_WIND
char V_9[] PROGMEM = "GUST";		//V_GUST
char V_10[] PROGMEM = "DIRECTON";	//V_DIRECTON
char V_11[] PROGMEM = "UV";		//V_UV
char V_12[] PROGMEM = "WEIGHT";		//V_WEIGHT
char V_13[] PROGMEM = "DISTANCE";	//V_DISTANCE
char V_14[] PROGMEM = "IMPEDANCE";	//V_IMPEDANCE
char V_15[] PROGMEM = "ARMED";		//V_ARMED
char V_16[] PROGMEM = "TRIPPED";	//V_TRIPPED
char V_17[] PROGMEM = "WATT";		//V_WATT
char V_18[] PROGMEM = "KWH";		//V_KWH
char V_19[] PROGMEM = "SCENE_ON";	//V_SCENE_ON
char V_20[] PROGMEM = "SCENE_OFF";	//V_SCENE_OFF
char V_21[] PROGMEM = "HEATER";		//V_HEATER
char V_22[] PROGMEM = "HEATER_SW";	//V_HEATER_SW
char V_23[] PROGMEM = "LIGHT_LEVEL";	//V_LIGHT_LEVEL
char V_24[] PROGMEM = "VAR1";		//V_VAR1
char V_25[] PROGMEM = "VAR2";		//V_VAR2
char V_26[] PROGMEM = "VAR3";		//V_VAR3
char V_27[] PROGMEM = "VAR4";		//V_VAR4
char V_28[] PROGMEM = "VAR5";		//V_VAR5
char V_29[] PROGMEM = "UP";		//V_UP
char V_30[] PROGMEM = "DOWN";		//V_DOWN
char V_31[] PROGMEM = "STOP";		//V_STOP
char V_32[] PROGMEM = "IR_SEND";	//V_IR_SEND
char V_33[] PROGMEM = "IR_RECEIVE";	//V_IR_RECEIVE
char V_34[] PROGMEM = "FLOW";		//V_FLOW
char V_35[] PROGMEM = "VOLUME";		//V_VOLUME
char V_36[] PROGMEM = "LOCK_STATUS";	//V_LOCK_STATUS
char V_37[] PROGMEM = "DUST_LEVEL";	//V_DUST_LEVEL
char V_38[] PROGMEM = "VOLTAGE";	//V_VOLTAGE
char V_39[] PROGMEM = "CURRENT";	//V_CURRENT
char V_40[] PROGMEM = "";		//
char V_41[] PROGMEM = "";		//
char V_42[] PROGMEM = "";		//
char V_43[] PROGMEM = "";		//
char V_44[] PROGMEM = "";		//
char V_45[] PROGMEM = "";		//
char V_46[] PROGMEM = "";		//
char V_47[] PROGMEM = "";		//
char V_48[] PROGMEM = "";		//
char V_49[] PROGMEM = "";		//
char V_50[] PROGMEM = "";		//
char V_51[] PROGMEM = "";		//
char V_52[] PROGMEM = "";		//
char V_53[] PROGMEM = "";		//
char V_54[] PROGMEM = "";		//
char V_55[] PROGMEM = "";		//
char V_56[] PROGMEM = "";		//
char V_57[] PROGMEM = "";		//
char V_58[] PROGMEM = "";		//
char V_59[] PROGMEM = "";		//
char V_60[] PROGMEM = "DEFAULT";	//Custom for MQTTGateway
char V_61[] PROGMEM = "SKETCH_NAME";	//Custom for MQTTGateway
char V_62[] PROGMEM = "SKETCH_VERSION"; //Custom for MQTTGateway
char V_63[] PROGMEM = "UNKNOWN"; 	//Custom for MQTTGateway

//////////////////////////////////////////////////////////////////

PROGMEM const char *vType[] = {
	V_0, V_1, V_2, V_3, V_4, V_5, V_6, V_7, V_8, V_9, V_10,
	V_11, V_12, V_13, V_14, V_15, V_16, V_17, V_18, V_19, V_20,
	V_21, V_22, V_23, V_24, V_25, V_26, V_27, V_28, V_29, V_30,
	V_31, V_32, V_33, V_34, V_35, V_36, V_37, V_38, V_39, V_40,
	V_41, V_42, V_43, V_44, V_45, V_46, V_47, V_48, V_49, V_50,
	V_51, V_52, V_53, V_54, V_55, V_56, V_57, V_58, V_59, V_60,
	V_61, V_62, V_63
};



#define S_FIRSTCUSTOM 60
#define V_TOTAL (sizeof(vType)/sizeof(char *))-1

#endif

#define S_FIRSTCUSTOM 60
#define V_UNKNOWN 63
