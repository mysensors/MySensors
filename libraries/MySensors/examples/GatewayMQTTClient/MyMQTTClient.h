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

//////////////////////////////////////////////////////////////////

#ifdef DEBUG
#define TCPDUMP					// Dump TCP packages.
#endif
#define MQTT_FIRST_SENSORID	20  		// If you want manually configured nodes below this value. 255 = Disable
#define MQTT_LAST_SENSORID	254 		// 254 is max! 255 reserved.
#define MQTT_PREFIX	"MyMQTT"	        // First prefix in MQTT tree, keep short!
#define MQTT_TOPIC_MASK "MyMQTT/+/+/+/+"        // Topic mask the client will subscribe to. This ensures that it will not receive any sensor data send by sensors
                                                // the following topic structure is assumed: MyMQTT/<node-id>/<child_id>/<sensor-type>/<cmd>
#define MQTT_SEND_SUBSCRIPTION 1		// Send empty payload (request) to node upon MQTT client subscribe request.
#define MQTT_CMD_SET "set"                      // command (see MQTT_TOPIC_MASK); mapped to C_SET command type of sensor network
#define MQTT_CMD_GET "get"                      // command (see MQTT_TOPIC_MASK); mapped to C_REQ command type of sensor network


#define MQTT_AUTH_REQUIRED                      // if your broker has anonymous access uncomment
#define MQTT_USER "user"                    // username on MQTT broker
#define MQTT_PWD  "pwd"                        // password for MQTT username above
#define MQTT_CONN_ID "MySensors"               // passed as id to the MQTT broker


// NOTE above : Beware to check if there is any length on payload in your incommingMessage code:
// Example: if (msg.type==V_LIGHT && strlen(msg.getString())>0) otherwise the code might do strange things.

//////////////////////////////////////////////////////////////////


char VAR_0[] PROGMEM = "TEMP";		//V_TEMP
char VAR_1[] PROGMEM = "HUM";		//V_HUM
char VAR_2[] PROGMEM = "LIGHT";		//V_LIGHT
char VAR_3[] PROGMEM = "DIMMER";		//V_DIMMER
char VAR_4[] PROGMEM = "PRESSURE";	//V_PRESSURE
char VAR_5[] PROGMEM = "FORECAST";	//V_FORECAST
char VAR_6[] PROGMEM = "RAIN";		//V_RAIN
char VAR_7[] PROGMEM = "RAINRATE";	//V_RAINRATE
char VAR_8[] PROGMEM = "WIND";		//V_WIND
char VAR_9[] PROGMEM = "GUST";		//V_GUST
char VAR_10[] PROGMEM = "DIRECTON";	//V_DIRECTON
char VAR_11[] PROGMEM = "UV";		//V_UV
char VAR_12[] PROGMEM = "WEIGHT";		//V_WEIGHT
char VAR_13[] PROGMEM = "DISTANCE";	//V_DISTANCE
char VAR_14[] PROGMEM = "IMPEDANCE";	//V_IMPEDANCE
char VAR_15[] PROGMEM = "ARMED";		//V_ARMED
char VAR_16[] PROGMEM = "TRIPPED";	//V_TRIPPED
char VAR_17[] PROGMEM = "WATT";		//V_WATT
char VAR_18[] PROGMEM = "KWH";		//V_KWH
char VAR_19[] PROGMEM = "SCENE_ON";	//V_SCENE_ON
char VAR_20[] PROGMEM = "SCENE_OFF";	//V_SCENE_OFF
char VAR_21[] PROGMEM = "HEATER";		//V_HEATER
char VAR_22[] PROGMEM = "HEATER_SW";	//V_HEATER_SW
char VAR_23[] PROGMEM = "LIGHT_LEVEL";	//V_LIGHT_LEVEL
char VAR_24[] PROGMEM = "VAR1";		//V_VAR1
char VAR_25[] PROGMEM = "VAR2";		//V_VAR2
char VAR_26[] PROGMEM = "VAR3";		//V_VAR3
char VAR_27[] PROGMEM = "VAR4";		//V_VAR4
char VAR_28[] PROGMEM = "VAR5";		//V_VAR5
char VAR_29[] PROGMEM = "UP";		//V_UP
char VAR_30[] PROGMEM = "DOWN";		//V_DOWN
char VAR_31[] PROGMEM = "STOP";		//V_STOP
char VAR_32[] PROGMEM = "IR_SEND";	//V_IR_SEND
char VAR_33[] PROGMEM = "IR_RECEIVE";	//V_IR_RECEIVE
char VAR_34[] PROGMEM = "FLOW";		//V_FLOW
char VAR_35[] PROGMEM = "VOLUME";		//V_VOLUME
char VAR_36[] PROGMEM = "LOCK_STATUS";	//V_LOCK_STATUS
char VAR_37[] PROGMEM = "LEVEL";	// S_DUST, S_LIGHT_LEVEL, ...
char VAR_38[] PROGMEM = "VOLTAGE";	//V_VOLTAGE
char VAR_39[] PROGMEM = "CURRENT";	//V_CURRENT
char VAR_40[] PROGMEM = "RGB";		//V_RGB
char VAR_41[] PROGMEM = "";		//
char VAR_42[] PROGMEM = "";		//
char VAR_43[] PROGMEM = "";		//
char VAR_44[] PROGMEM = "";		//
char VAR_45[] PROGMEM = "";		//
char VAR_46[] PROGMEM = "";		//
char VAR_47[] PROGMEM = "";		//
char VAR_48[] PROGMEM = "";		//
char VAR_49[] PROGMEM = "";		//
char VAR_50[] PROGMEM = "";		//
char VAR_51[] PROGMEM = "";		//
char VAR_52[] PROGMEM = "";		//
char VAR_53[] PROGMEM = "";		//
char VAR_54[] PROGMEM = "";		//
char VAR_55[] PROGMEM = "";		//
char VAR_56[] PROGMEM = "";		//
char VAR_57[] PROGMEM = "";		//
char VAR_58[] PROGMEM = "";		//
char VAR_59[] PROGMEM = "";		//
char VAR_60[] PROGMEM = "Started!\n";	//Custom for MQTTGateway
char VAR_61[] PROGMEM = "SKETCH_NAME";	//Custom for MQTTGateway
char VAR_62[] PROGMEM = "SKETCH_VERSION"; //Custom for MQTTGateway
char VAR_63[] PROGMEM = "UNKNOWN"; 	//Custom for MQTTGateway

//////////////////////////////////////////////////////////////////

PROGMEM const char *VAR_Type[] =
{ VAR_0, VAR_1, VAR_2, VAR_3, VAR_4, VAR_5, VAR_6, VAR_7, VAR_8, VAR_9, VAR_10, VAR_11, VAR_12, VAR_13,
    VAR_14, VAR_15, VAR_16, VAR_17, VAR_18, VAR_19, VAR_20, VAR_21, VAR_22, VAR_23, VAR_24, VAR_25,
    VAR_26, VAR_27, VAR_28, VAR_29, VAR_30, VAR_31, VAR_32, VAR_33, VAR_34, VAR_35, VAR_36, VAR_37,
    VAR_38, VAR_39, VAR_40, VAR_41, VAR_42, VAR_43, VAR_44, VAR_45, VAR_46, VAR_47, VAR_48, VAR_49,
    VAR_50, VAR_51, VAR_52, VAR_53, VAR_54, VAR_55, VAR_56, VAR_57, VAR_58, VAR_59, VAR_60, VAR_61,
    VAR_62, VAR_63
};

char mqtt_prefix[] PROGMEM = MQTT_PREFIX;

#define S_FIRSTCUSTOM 60
#define TYPEMAXLEN 20
#define VAR_TOTAL (sizeof(VAR_Type)/sizeof(char *))-1


#define EEPROM_LATEST_NODE_ADDRESS ((uint8_t)EEPROM_LOCAL_CONFIG_ADDRESS)

//#define DSRTC // RTC Dallas support -> disable debug and use isp programmer to free memory
#endif
