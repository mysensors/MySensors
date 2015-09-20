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
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Created by Daniel Wiegert <daniel.wiegert@gmail.com>
 * 
 * DESCRIPTION
 * MyMQTT Broker Gateway 0.1b
 * Latest instructions found here:
 * http://www.mysensors.org/build/mqtt_gateway
 * http://www.mysensors.org/build/ethernet_gateway
 * 
 * Change below; TCP_IP, TCP_PORT, TCP_MAC
 * This will listen on your selected TCP_IP:TCP_PORT below, Please change TCP_MAC your liking also.
 * 1 -> NOTE: Keep first byte at x2, x6, xA or xE (replace x with any hex value) for using Local Ranges.
 * 2 You can use standard pin set-up as MySensors recommends or if you own a IBOARD you may change
 *	the radio-pins below if you hardware mod your iBoard. see [URL BELOW] for more details.
 *	http://forum.mysensors.org/topic/224/iboard-cheap-single-board-ethernet-arduino-with-radio/5
 *
 * Don't forget to look at the definitions in MyMQTT.h!
 *
 *	define TCPDUMP and connect serial interface if you have problems, please write on
 *	http://forum.mysensors.org/ and explain your problem, include serial output. Don't forget to
 *	turn on DEBUG in libraries\MySensors\MyConfig.h also.
 *
 *	MQTT_FIRST_SENSORID is for 'DHCP' server in MyMQTT. You may limit the ID's with FIRST and LAST definition.
 *	If you want your manually configured below 20 set MQTT_FIRST_SENSORID to 20.
 *	To disable: set MQTT_FIRST_SENSORID to 255.
 *
 *	MQTT_BROKER_PREFIX is the leading prefix for your nodes. This can be only one char if like.
 *
 *	MQTT_SEND_SUBSCRIPTION is if you want the MyMQTT to send a empty payload message to your nodes.
 *	This can be useful if you want to send latest state back to the MQTT client. Just check if incoming
 *	message has any length or not.
 *	Example: if (msg.type==V_LIGHT && strlen(msg.getString())>0) otherwise the code might do strange things.
 *
 * (*) Address-layout is : [MQTT_BROKER_PREFIX]/[NodeID]/[SensorID]/V_[SensorType]
 *	NodeID and SensorID is uint8 (0-255) number.
 *	Last segment is translation of the sensor type, look inside MyMQTT.cpp for the definitions.
 *	User can change this to their needs. We have also left some space for custom types.
 *
 * Special: (sensor 255 reserved for special commands)
 * You can receive a node sketch name with MyMQTT/20/255/V_Sketch_name (or version with _version)
 *
 * To-do:
 * Special commands : clear or set EEPROM Values, Send REBOOT and Receive reboot for MyMQTT itself.
 * Be able to send ACK so client returns the data being sent.
 * ... Please come with ideas!
 * What to do with publish messages.
 *
 * Test in more MQTT clients, So far tested in openhab and MyMQTT for Android (Not my creation)
 * - http://www.openhab.org/
 * - https://play.google.com/store/apps/details?id=at.tripwire.mqtt.client&hl=en
 * ... Please notify me if you use this broker with other software.
 * 
 *  How to set-up Openhab and MQTTGateway:
 * http://forum.mysensors.org/topic/303/mqtt-broker-gateway
 */

#include <DigitalIO.h>
#include <SPI.h>

#include <MySigningNone.h>
#include <MyTransportRFM69.h>
#include <MyTransportNRF24.h>
#include <MyHwATMega328.h>
#include <MySigningAtsha204Soft.h>
#include <MySigningAtsha204.h>

#include <MySensor.h>
#include <MsTimer2.h>
#include <Ethernet.h>
#include "MyMQTT.h"


#define INCLUSION_MODE_TIME 1 // Number of minutes inclusion mode is enabled
#define INCLUSION_MODE_PIN  3 // Digital pin used for inclusion mode button

// * Use this for IBOARD modded to use standard MISO/MOSI/SCK, see note *1 above!
/*
#define RADIO_CE_PIN        3			// radio chip enable
#define RADIO_SPI_SS_PIN    8			// radio SPI serial select
#define RADIO_ERROR_LED_PIN A2  		// Error led pin
#define RADIO_RX_LED_PIN    A1  		// Receive led pin
#define RADIO_TX_LED_PIN    A0  		// the PCB, on board LED
*/

// * Use this for default configured pro mini / nano etc :
///*

#define RADIO_CE_PIN        5		// radio chip enable
#define RADIO_SPI_SS_PIN    6		// radio SPI serial select
#define RADIO_ERROR_LED_PIN 7		// Error led pin
#define RADIO_RX_LED_PIN    8		// Receive led pin
#define RADIO_TX_LED_PIN    9		// the PCB, on board LED*/

#define TCP_PORT 1883						// Set your MQTT Broker Listening port.
IPAddress TCP_IP ( 192, 168, 0, 234 );				// Configure your static ip-address here
byte TCP_MAC[] = { 0x02, 0xDE, 0xAD, 0x00, 0x00, 0x42 };	// Mac-address - You should change this! see note *2 above!

//////////////////////////////////////////////////////////////////

// NRFRF24L01 radio driver (set low transmit power by default) 
MyTransportNRF24 transport(RADIO_CE_PIN, RADIO_SPI_SS_PIN, RF24_PA_LEVEL_GW);  
//MyTransportRFM69 transport;

// Message signing driver (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
//MySigningNone signer;
//MySigningAtsha204Soft signer;
//MySigningAtsha204 signer;

// Hardware profile 
MyHwATMega328 hw;

// Construct MySensors library (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
MySensor gw(transport, hw /*, signer*/);


EthernetServer server = EthernetServer(TCP_PORT);
EthernetClient *currentClient = NULL;
MyMessage msg;
char convBuf[MAX_PAYLOAD*2+1];
char broker[] PROGMEM = MQTT_BROKER_PREFIX;
bool MQTTClientConnected = false;
uint8_t buffsize;
char buffer[MQTT_MAX_PACKET_SIZE];
volatile uint8_t countRx;
volatile uint8_t countTx;
volatile uint8_t countErr;


void writeEthernet(const char *writeBuffer, uint8_t *writeSize) {
#ifdef TCPDUMP
	Serial.print(">>");
	char buf[4];
	for (uint8_t a=0; a<*writeSize; a++) { sprintf(buf,"%02X ",(uint8_t)writeBuffer[a]);  Serial.print(buf); } Serial.println("");
#endif
	server.write((const uint8_t *)writeBuffer, *writeSize);
}


void processEthernetMessages() {
  char inputString[MQTT_MAX_PACKET_SIZE] = "";
  byte inputSize = 0;
  byte readCnt = 0;
  byte length = 0;

  EthernetClient client = server.available();
  if (client) {
    while (client.available()) {
      // Save the current client we are talking with
      currentClient = &client;
      byte inByte = client.read();
      readCnt++;

      if (inputSize < MQTT_MAX_PACKET_SIZE) {
        inputString[inputSize] = (char)inByte;
        inputSize++;
      }

      if (readCnt == 2) {
        length = (inByte & 127) * 1;
      }
      if (readCnt == (length+2)) {
        break;
      }
    }
#ifdef TCPDUMP
    Serial.print("<<");
    char buf[4];
    for (byte a=0; a<inputSize; a++) { sprintf(buf, "%02X ", (byte)inputString[a]); Serial.print(buf); } Serial.println();
#endif
    processMQTTMessage(inputString, inputSize);
    currentClient = NULL;
  }
}




void incomingMessage(const MyMessage &message) {
   rxBlink(1);
   sendMQTT(message);
} 



void setup()  
{ 
  Ethernet.begin(TCP_MAC, TCP_IP);

  
  countRx = 0;
  countTx = 0;
  countErr = 0;

  // Setup led pins
  pinMode(RADIO_RX_LED_PIN, OUTPUT);
  pinMode(RADIO_TX_LED_PIN, OUTPUT);
  pinMode(RADIO_ERROR_LED_PIN, OUTPUT);
  digitalWrite(RADIO_RX_LED_PIN, LOW);
  digitalWrite(RADIO_TX_LED_PIN, LOW);
  digitalWrite(RADIO_ERROR_LED_PIN, LOW);

 
  // Set initial state of leds
  digitalWrite(RADIO_RX_LED_PIN, HIGH);
  digitalWrite(RADIO_TX_LED_PIN, HIGH);
  digitalWrite(RADIO_ERROR_LED_PIN, HIGH);


  // Add led timer interrupt
  MsTimer2::set(300, ledTimersInterrupt);
  MsTimer2::start();


  // give the Ethernet interface a second to initialize
  delay(1000);


  // Initialize gateway at maximum PA level, channel 70 and callback for write operations 
  gw.begin(incomingMessage, 0, true, 0);  

  // start listening for clients
  server.begin();
  
  Serial.println("Ok!");
}

void loop()
{
  gw.process();  
  
  processEthernetMessages();
}





inline MyMessage& build (MyMessage &msg, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.destination = destination;
	msg.sender = GATEWAY_ADDRESS;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}

char *getType(char *b, const char **index) {
	char *q = b;
	char *p = (char *)pgm_read_word(index);
	while (*q++ = pgm_read_byte(p++));
	*q=0;
	return b;
}

void processMQTTMessage(char *inputString, uint8_t inputPos) {
	char *str, *p;
	uint8_t i = 0;
	buffer[0]= 0;
	buffsize = 0;
	(void)inputPos;

	if ((uint8_t)inputString[0] >> 4 == MQTTCONNECT) {
		buffer[buffsize++] = MQTTCONNACK << 4;
		buffer[buffsize++] = 0x02;			// Remaining length
		buffer[buffsize++] = 0x00;			// Connection accepted
		buffer[buffsize++] = 0x00;			// Reserved
		MQTTClientConnected=true;			// We have connection!
	}
	if ((uint8_t)inputString[0] >> 4 == MQTTPINGREQ) {
		buffer[buffsize++] = MQTTPINGRESP << 4;
		buffer[buffsize++] = 0x00;
	}
	if ((uint8_t)inputString[0] >> 4 == MQTTSUBSCRIBE) {
		buffer[buffsize++] = MQTTSUBACK << 4;		// Just ack everything, we actually dont really care!
		buffer[buffsize++] = 0x03;			// Remaining length
		buffer[buffsize++] = (uint8_t)inputString[2];	// Message ID MSB
		buffer[buffsize++] = (uint8_t)inputString[3];	// Message ID LSB
		buffer[buffsize++] = MQTTQOS0;			// QOS level
	}
	if ((uint8_t)inputString[0] >> 4 == MQTTUNSUBSCRIBE) {
		buffer[buffsize++] = MQTTUNSUBACK << 4;
		buffer[buffsize++] = 0x02;			// Remaining length
		buffer[buffsize++] = (uint8_t)inputString[2];	// Message ID MSB
		buffer[buffsize++] = (uint8_t)inputString[3];	// Message ID LSB
	}
	if ((uint8_t)inputString[0] >> 4 == MQTTDISCONNECT) {
		MQTTClientConnected=false;			// We lost connection!
	}
	if (buffsize > 0) {
		writeEthernet(buffer,&buffsize);
	}

	// We publish everything we get, we dont care if its subscribed or not!
	if ((uint8_t)inputString[0] >> 4 == MQTTPUBLISH || (MQTT_SEND_SUBSCRIPTION && (uint8_t)inputString[0] >> 4 == MQTTSUBSCRIBE)) {
		buffer[0]= 0;
		buffsize = 0;
		// Cut out address and payload depending on message type.
		if ((uint8_t)inputString[0] >> 4 == MQTTSUBSCRIBE) {
			strncat(buffer,inputString+6,inputString[5]);
		} else {
			strncat(buffer,inputString+4,inputString[3]);
		}

#ifdef DEBUG
		Serial.println(buffer);
#endif
		// TODO: Check if we should send ack or not.
		for (str = strtok_r(buffer,"/",&p) ; str && i<4 ; str = strtok_r(NULL,"/",&p)) {
			if (i == 0) {
				if (strcmp_P(str,broker)!=0) {	//look for MQTT_BROKER_PREFIX
					return;			//Message not for us or malformatted!
				}
			} else if (i==1) {
				msg.destination = atoi(str);	//NodeID
			} else if (i==2) {
				msg.sensor = atoi(str);		//SensorID
			} else if (i==3) {
				unsigned char match=255;			//SensorType
#ifdef MQTT_TRANSLATE_TYPES				

				for (uint8_t j=0; strcpy_P(convBuf, (char*)pgm_read_word(&(vType[j]))) ; j++) {
					if (strcmp((char*)&str[2],convBuf)==0) { //Strip V_ and compare
						match=j;
						break;
					}
					if (j >= V_TOTAL)  break;
				}

#endif
                                if ( atoi(str)!=0 || (str[0]=='0' && str[1] =='\0') ) {
					match=atoi(str);
				}

				if (match==255) {
					match=V_UNKNOWN;
 				}
				msg.type = match;
			}
			i++;
		}						//Check if packge has payload
		if ((uint8_t)inputString[1] > (uint8_t)(inputString[3]+2) && !((uint8_t)inputString[0] >> 4 == MQTTSUBSCRIBE)) {
			strcpy(convBuf,inputString+(inputString[3]+4));
			msg.set(convBuf);			//Payload
		} else {
			msg.set("");				//No payload
		}
		txBlink(1);
		if (!gw.sendRoute(build(msg, msg.destination, msg.sensor, C_SET, msg.type, 0))) errBlink(1);

	}
}

void sendMQTT(const MyMessage &inMsg) {
        MyMessage msg = inMsg;
	buffsize = 0;
	if (!MQTTClientConnected) return;			//We have no connections - return
	if (msg.isAck()) {
//		if (msg.sender==255 && mGetCommand(msg)==C_INTERNAL && msg.type==I_ID_REQUEST) {
// TODO: sending ACK request on id_response fucks node up. doesn't work.
// The idea was to confirm id and save to EEPROM_LATEST_NODE_ADDRESS.
//  }
	} else {
		// we have to check every message if its a newly assigned id or not.
		// Ack on I_ID_RESPONSE does not work, and checking on C_PRESENTATION isn't reliable.
		uint8_t newNodeID = gw.loadState(EEPROM_LATEST_NODE_ADDRESS)+1;
		if (newNodeID <= MQTT_FIRST_SENSORID) newNodeID = MQTT_FIRST_SENSORID;
		if (msg.sender==newNodeID) {
			gw.saveState(EEPROM_LATEST_NODE_ADDRESS,newNodeID);
		}
		if (mGetCommand(msg)==C_INTERNAL) {
			if (msg.type==I_CONFIG) {
				txBlink(1);
				if (!gw.sendRoute(build(msg, msg.sender, 255, C_INTERNAL, I_CONFIG, 0).set(MQTT_UNIT))) errBlink(1);
				return;
			} else if (msg.type==I_ID_REQUEST && msg.sender==255) {
				uint8_t newNodeID = gw.loadState(EEPROM_LATEST_NODE_ADDRESS)+1;
				if (newNodeID <= MQTT_FIRST_SENSORID) newNodeID = MQTT_FIRST_SENSORID;
				if (newNodeID >= MQTT_LAST_SENSORID) return; // Sorry no more id's left :(
				txBlink(1);
				if (!gw.sendRoute(build(msg, msg.sender, 255, C_INTERNAL, I_ID_RESPONSE, 0).set(newNodeID))) errBlink(1);
				return;
			}
		}
		if (mGetCommand(msg)!=C_PRESENTATION) {
			if (mGetCommand(msg)==C_INTERNAL) msg.type=msg.type+(S_FIRSTCUSTOM-10);	//Special message
			buffer[buffsize++] = MQTTPUBLISH << 4;	// 0:
			buffer[buffsize++] = 0x09;		// 1: Remaining length with no payload, we'll set this later to correct value, buffsize -2
			buffer[buffsize++] = 0x00;		// 2: Length MSB (Remaing length can never exceed ff,so MSB must be 0!)
			buffer[buffsize++] = 0x08;		// 3: Length LSB (ADDR), We'll set this later
			strcpy_P(buffer+4, broker);
			buffsize+=strlen_P(broker);
#ifdef MQTT_TRANSLATE_TYPES
			if (msg.type > V_TOTAL) msg.type=V_UNKNOWN;// If type > defined types set to unknown.
 			buffsize+=sprintf(&buffer[buffsize],"/%i/%i/V_%s",msg.sender,msg.sensor,getType(convBuf, &vType[msg.type]));
#else
			buffsize+=sprintf(&buffer[buffsize],"/%i/%i/%i",msg.sender,msg.sensor,msg.type);
#endif
			buffer[3]=buffsize-4;			// Set correct address length on byte 4.
#ifdef DEBUG
			Serial.println((char*)&buffer[4]);
#endif
			msg.getString(convBuf);
			for (uint8_t a=0; a<strlen(convBuf); a++) {// Payload
				buffer[buffsize++] = convBuf[a];
			}
			buffer[1]=buffsize-2;			// Set correct Remaining length on byte 2.
			writeEthernet(buffer,&buffsize);
		}
	}
}


void ledTimersInterrupt() {
  if(countRx && countRx != 255) {
    // switch led on
    digitalWrite(RADIO_RX_LED_PIN, LOW);
  } else if(!countRx) {
     // switching off
     digitalWrite(RADIO_RX_LED_PIN, HIGH);
   }
   if(countRx != 255) { countRx--; }

  if(countTx && countTx != 255) {
    // switch led on
    digitalWrite(RADIO_TX_LED_PIN, LOW);
  } else if(!countTx) {
     // switching off
     digitalWrite(RADIO_TX_LED_PIN, HIGH);
   }
   if(countTx != 255) { countTx--; }

  if(countErr && countErr != 255) {
    // switch led on
    digitalWrite(RADIO_ERROR_LED_PIN, LOW);
  } else if(!countErr) {
     // switching off
     digitalWrite(RADIO_ERROR_LED_PIN, HIGH);
   }
   if(countErr != 255) { countErr--; }

}

void rxBlink(uint8_t cnt) {
  if(countRx == 255) { countRx = cnt; }
}
void txBlink(uint8_t cnt) {
  if(countTx == 255) { countTx = cnt; }
}
void errBlink(uint8_t cnt) {
  if(countErr == 255) { countErr = cnt; }
}

