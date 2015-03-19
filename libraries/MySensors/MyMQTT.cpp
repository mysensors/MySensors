/*
The MySensors library adds a new layer on top of the RF24 library.
It handles radio network routing, relaying and ids.

Created by Henrik Ekblad <henrik.ekblad@gmail.com>
Modified by Daniel Wiegert
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.
*/

#include "MyMQTT.h"
#include "utility/MsTimer2.h"

char V_0[] PROGMEM = "TEMP";		//V_TEMP
char V_1[] PROGMEM = "HUM";			//V_HUM
char V_2[] PROGMEM = "LIGHT";		//V_LIGHT
char V_3[] PROGMEM = "DIMMER";		//V_DIMMER
char V_4[] PROGMEM = "PRESSURE";	//V_PRESSURE
char V_5[] PROGMEM = "FORECAST";	//V_FORECAST
char V_6[] PROGMEM = "RAIN";		//V_RAIN
char V_7[] PROGMEM = "RAINRATE";	//V_RAINRATE
char V_8[] PROGMEM = "WIND";		//V_WIND
char V_9[] PROGMEM = "GUST";		//V_GUST
char V_10[] PROGMEM = "DIRECTON";	//V_DIRECTON
char V_11[] PROGMEM = "UV";			//V_UV
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
char V_23[] PROGMEM = "LIGHT_LEVEL";//V_LIGHT_LEVEL
char V_24[] PROGMEM = "VAR1";		//V_VAR1
char V_25[] PROGMEM = "VAR2";		//V_VAR2
char V_26[] PROGMEM = "VAR3";		//V_VAR3
char V_27[] PROGMEM = "VAR4";		//V_VAR4
char V_28[] PROGMEM = "VAR5";		//V_VAR5
char V_29[] PROGMEM = "UP";			//V_UP
char V_30[] PROGMEM = "DOWN";		//V_DOWN
char V_31[] PROGMEM = "STOP";		//V_STOP
char V_32[] PROGMEM = "IR_SEND";	//V_IR_SEND
char V_33[] PROGMEM = "IR_RECEIVE";	//V_IR_RECEIVE
char V_34[] PROGMEM = "FLOW";		//V_FLOW
char V_35[] PROGMEM = "VOLUME";		//V_VOLUME
char V_36[] PROGMEM = "LOCK_STATUS";//V_LOCK_STATUS
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
char V_60[] PROGMEM = "Started!\n";		//Custom for MQTTGateway
char V_61[] PROGMEM = "SKETCH_NAME";	//Custom for MQTTGateway
char V_62[] PROGMEM = "SKETCH_VERSION"; //Custom for MQTTGateway
char V_63[] PROGMEM = "UNKNOWN"; 		//Custom for MQTTGateway

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

char broker[] PROGMEM = MQTT_BROKER_PREFIX;

#define S_FIRSTCUSTOM 60
#define TYPEMAXLEN 20
#define V_TOTAL (sizeof(vType)/sizeof(char *))-1

extern volatile uint8_t countRx;
extern volatile uint8_t countTx;
extern volatile uint8_t countErr;
extern uint8_t pinRx;
extern uint8_t pinTx;
extern uint8_t pinEr;

MyMQTT::MyMQTT(uint8_t _cepin, uint8_t _cspin) :
MySensor(_cepin, _cspin) {

}

inline MyMessage& build (MyMessage &msg, uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.destination = destination;
	msg.sender = sender;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}

char *MyMQTT::getType(char *b, const char **index) {
	char *q = b;
	char *p = (char *)pgm_read_word(index);
	while (*q++ = pgm_read_byte(p++));
	*q=0;
	return b;
}

void MyMQTT::begin(rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate, void (*inDataCallback)
			(const char *, uint8_t *), uint8_t _rx, uint8_t _tx, uint8_t _er) {
	Serial.begin(BAUD_RATE);
	repeaterMode = true;
	isGateway = true;
	MQTTClients = 0;

	setupRepeaterMode();
	dataCallback = inDataCallback;

	nc.nodeId = 0;
	nc.distance = 0;

	// Start up the radio library
	setupRadio(paLevel, channel, dataRate);
	RF24::openReadingPipe(WRITE_PIPE, BASE_RADIO_ID);
	RF24::openReadingPipe(CURRENT_NODE_PIPE, BASE_RADIO_ID);
	RF24::startListening();

	pinRx = _rx;
	pinMode(pinRx, OUTPUT);
	pinTx = _tx;
	pinMode(pinTx, OUTPUT);
	pinEr = _er;
	pinMode(pinEr, OUTPUT);

	MsTimer2::set(200, ledTimersInterrupt);
	MsTimer2::start();

	Serial.print(getType(convBuf, &vType[S_FIRSTCUSTOM]));
}

void MyMQTT::processRadioMessage() {
	if (process()) {
		// A new message was received from one of the sensors
		MyMessage message = getLastMessage();
		rxBlink(1);

		if (msg.isAck()) {
			Serial.println("msg is ack!");
			if (msg.sender == 255 && mGetCommand(msg) == C_INTERNAL && msg.type == I_ID_REQUEST) {
				// TODO: sending ACK request on id_response fucks node up. doesn't work.
				// The idea was to confirm id and save to EEPROM_LATEST_NODE_ADDRESS.
			}
		} else {
			// we have to check every message if its a newly assigned id or not.
			// Ack on I_ID_RESPONSE does not work, and checking on C_PRESENTATION isn't reliable.
			uint8_t newNodeID = loadState(EEPROM_LATEST_NODE_ADDRESS)+1;
			if (newNodeID <= MQTT_FIRST_SENSORID) {
				newNodeID = MQTT_FIRST_SENSORID;
			}
			if (msg.sender == newNodeID) {
				saveState(EEPROM_LATEST_NODE_ADDRESS,newNodeID);
			}

			if (mGetCommand(msg) == C_INTERNAL) {
				if (msg.type == I_CONFIG) {
					txBlink(1);
					if (!sendRoute(build(msg, GATEWAY_ADDRESS, msg.sender, 255, C_INTERNAL, I_CONFIG, 0).set("M"))) {
						errBlink(1);
					}
				} else if (msg.type == I_ID_REQUEST && msg.sender == 255) {
					uint8_t newNodeID = loadState(EEPROM_LATEST_NODE_ADDRESS)+1;
					if (newNodeID <= MQTT_FIRST_SENSORID) {
						newNodeID = MQTT_FIRST_SENSORID;
					}
					if (newNodeID >= MQTT_LAST_SENSORID) {
						// Sorry no more id's left :(
						newNodeID = AUTO;
					}
					txBlink(1);
					if (!sendRoute(build(msg, GATEWAY_ADDRESS, msg.sender, 255, C_INTERNAL, I_ID_RESPONSE, 0).set(newNodeID))) {
						errBlink(1);
					}
				}
			} else if (mGetCommand(msg)!= C_PRESENTATION) {
				// Pass along the message from sensors to MQTT
				SendMQTT(message);
			}
		}
	}
}

void MyMQTT::processMQTTMessage(char *inputString, uint8_t inputPos) {
	char *str, *p, *payload=NULL;
	uint8_t i = 0;
	buffer[0]= 0;
	buffsize = 0;
	uint8_t mqttMsgType = (uint8_t)inputString[0] >> 4;

	if (mqttMsgType == MQTTCONNECT) {
		buffer[buffsize++] = MQTTCONNACK << 4;
		buffer[buffsize++] = 0x02;			// Remaining length
		buffer[buffsize++] = 0x00;			// Connection accepted
		buffer[buffsize++] = 0x00;			// Reserved
		MQTTClients++;						// We have a new client connected!
	} else if (mqttMsgType == MQTTPINGREQ) {
		buffer[buffsize++] = MQTTPINGRESP << 4;
		buffer[buffsize++] = 0x00;
	} else if (mqttMsgType == MQTTSUBSCRIBE) {
		buffer[buffsize++] = MQTTSUBACK << 4;			// Just ack everything, we actually dont really care!
		buffer[buffsize++] = 0x03;						// Remaining length
		buffer[buffsize++] = (uint8_t)inputString[2];	// Message ID MSB
		buffer[buffsize++] = (uint8_t)inputString[3];	// Message ID LSB
		buffer[buffsize++] = MQTTQOS0;					// QOS level
	} else if (mqttMsgType == MQTTUNSUBSCRIBE) {
		buffer[buffsize++] = MQTTUNSUBACK << 4;
		buffer[buffsize++] = 0x02;						// Remaining length
		buffer[buffsize++] = (uint8_t)inputString[2];	// Message ID MSB
		buffer[buffsize++] = (uint8_t)inputString[3];	// Message ID LSB
	} else if (mqttMsgType == MQTTDISCONNECT) {
		MQTTClients--;		// Client disconnected!
	}

	if (buffsize > 0) {
		dataCallback(buffer, &buffsize);
	}

	// We publish everything we get, we dont care if its subscribed or not!
	if (mqttMsgType == MQTTPUBLISH || (MQTT_SEND_SUBSCRIPTION && mqttMsgType == MQTTSUBSCRIBE)) {
		buffer[0] = 0;
		buffsize = 0;
		// Cut out address and payload depending on message type.
		if (mqttMsgType == MQTTSUBSCRIBE) {
			strncat(buffer, inputString+6, inputString[5]);
		} else {
			strncat(buffer, inputString+4, inputString[3]);
		}

		// TODO: Check if we should send ack or not.
		for (str = strtok_r(buffer, "/", &p) ; str && i<4 ; str = strtok_r(NULL, "/", &p)) {
			if (i == 0) {
				//look for MQTT_BROKER_PREFIX
				if (strcmp_P(str, broker) != 0) {
					//Message not for us or malformatted!
					return;
				}
			} else if (i==1) {
				//NodeID
				msg.destination = atoi(str);
			} else if (i==2) {
				//SensorID
				msg.sensor = atoi(str);
			} else if (i==3) {
				//SensorType
				char match=0;

				for (uint8_t j=0; strcpy_P(convBuf, (char*)pgm_read_word(&(vType[j]))) ; j++) {
					//Strip V_ and compare
					if (strcmp((char*)&str[2], convBuf)==0) {
						match=j;
						break;
					}
					if (j >= V_TOTAL) {	// No match found!
						match=V_TOTAL;	// last item.
						break;
					}
				}
				msg.type = match;
			}
			i++;
		}

		// Check if package has payload
		if (mqttMsgType == MQTTPUBLISH) {
			uint8_t length = (uint8_t)inputString[1] - (uint8_t)(inputString[3]+2);
			if (length && length < MAX_PAYLOAD*2) {
				// Payload
				memcpy(convBuf, inputString+(inputString[3]+4), length);
				convBuf[length] = 0;
				payload = convBuf;
			}
		}
		msg.set(payload);

		txBlink(1);
		if (!sendRoute(build(msg, GATEWAY_ADDRESS, msg.destination, msg.sensor, C_SET, msg.type, 0))) errBlink(1);
	}
}

void MyMQTT::SendMQTT(MyMessage &msg) {
	buffsize = 0;
	if (!MQTTClients) {
		//We have no clients connected - return
		return;
	}

	if (mGetCommand(msg) == C_INTERNAL) {
		//Special message
		msg.type = msg.type+(S_FIRSTCUSTOM-10);
	}

	buffer[buffsize++] = MQTTPUBLISH << 4;	// 0:
	buffer[buffsize++] = 0x09;		// 1: Remaining length with no payload, we'll set this later to correct value, buffsize -2
	buffer[buffsize++] = 0x00;		// 2: Length MSB (Remaing length can never exceed ff,so MSB must be 0!)
	buffer[buffsize++] = 0x08;		// 3: Length LSB (ADDR), We'll set this later
	if (msg.type > V_TOTAL) {
		// If type > defined types set to unknown.
		msg.type=V_TOTAL;
	}
	strcpy_P(buffer+4, broker);
	buffsize+=strlen_P(broker);
	buffsize+=sprintf(&buffer[buffsize], "/%i/%i/V_%s", msg.sender, msg.sensor, getType(convBuf, &vType[msg.type]));
	buffer[3]=buffsize-4;			// Set correct address length on byte 4.
#ifdef DEBUG
	Serial.println((char*)&buffer[4]);
#endif
	msg.getString(convBuf);
	for (uint8_t a=0; a<strlen(convBuf); a++) {	// Payload
		buffer[buffsize++] = convBuf[a];
	}
	buffer[1]=buffsize-2;			// Set correct Remaining length on byte 2.
	dataCallback(buffer, &buffsize);
}

void MyMQTT::rxBlink(uint8_t cnt) {
	if(countRx == 255) { countRx = cnt; }
}

void MyMQTT::txBlink(uint8_t cnt) {
	if(countTx == 255) { countTx = cnt; }
}

void MyMQTT::errBlink(uint8_t cnt) {
	if(countErr == 255) { countErr = cnt; }
}
