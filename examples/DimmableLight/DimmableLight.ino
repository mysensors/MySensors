/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
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
 * Version 1.0 - January 30, 2015 - Developed by GizMoCuz (Domoticz)
 *
 * DESCRIPTION
 * This sketch provides an example how to implement a Dimmable Light
 * It is pure virtual and it logs messages to the serial output
 * It can be used as a base sketch for actual hardware.
 * Stores the last light state and level in eeprom.
 *
 */

// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

#define CHILD_ID_LIGHT 1

#define EPROM_LIGHT_STATE 1
#define EPROM_DIMMER_LEVEL 2

#define LIGHT_OFF 0
#define LIGHT_ON 1

#define SN "Dimmable Light"
#define SV "1.0"

int16_t LastLightState=LIGHT_OFF;
int16_t LastDimValue=100;

MyMessage lightMsg(CHILD_ID_LIGHT, V_LIGHT);
MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);

void setup()
{
	//Retreive our last light state from the eprom
	int LightState=loadState(EPROM_LIGHT_STATE);
	if (LightState<=1) {
		LastLightState=LightState;
		int DimValue=loadState(EPROM_DIMMER_LEVEL);
		if ((DimValue>0)&&(DimValue<=100)) {
			//There should be no Dim value of 0, this would mean LIGHT_OFF
			LastDimValue=DimValue;
		}
	}

	//Here you actually switch on/off the light with the last known dim level
	SetCurrentState2Hardware();

	Serial.println( "Node ready to receive messages..." );
}

void presentation()
{
	// Send the Sketch Version Information to the Gateway
	sendSketchInfo(SN, SV);

	present(CHILD_ID_LIGHT, S_DIMMER );
}

void loop()
{
}

void receive(const MyMessage &message)
{
	if (message.type == V_LIGHT) {
		Serial.println( "V_LIGHT command received..." );

		int lstate= atoi( message.data );
		if ((lstate<0)||(lstate>1)) {
			Serial.println( "V_LIGHT data invalid (should be 0/1)" );
			return;
		}
		LastLightState=lstate;
		saveState(EPROM_LIGHT_STATE, LastLightState);

		if ((LastLightState==LIGHT_ON)&&(LastDimValue==0)) {
			//In the case that the Light State = On, but the dimmer value is zero,
			//then something (probably the controller) did something wrong,
			//for the Dim value to 100%
			LastDimValue=100;
			saveState(EPROM_DIMMER_LEVEL, LastDimValue);
		}

		//When receiving a V_LIGHT command we switch the light between OFF and the last received dimmer value
		//This means if you previously set the lights dimmer value to 50%, and turn the light ON
		//it will do so at 50%
	} else if (message.type == V_DIMMER) {
		Serial.println( "V_DIMMER command received..." );
		int dimvalue= atoi( message.data );
		if ((dimvalue<0)||(dimvalue>100)) {
			Serial.println( "V_DIMMER data invalid (should be 0..100)" );
			return;
		}
		if (dimvalue==0) {
			LastLightState=LIGHT_OFF;
		} else {
			LastLightState=LIGHT_ON;
			LastDimValue=dimvalue;
			saveState(EPROM_DIMMER_LEVEL, LastDimValue);
		}
	} else {
		Serial.println( "Invalid command received..." );
		return;
	}

	//Here you set the actual light state/level
	SetCurrentState2Hardware();
}

void SetCurrentState2Hardware()
{
	if (LastLightState==LIGHT_OFF) {
		Serial.println( "Light state: OFF" );
	} else {
		Serial.print( "Light state: ON, Level: " );
		Serial.println( LastDimValue );
	}

	//Send current state to the controller
	SendCurrentState2Controller();
}

void SendCurrentState2Controller()
{
	if ((LastLightState==LIGHT_OFF)||(LastDimValue==0)) {
		send(dimmerMsg.set((int16_t)0));
	} else {
		send(dimmerMsg.set(LastDimValue));
	}
}
