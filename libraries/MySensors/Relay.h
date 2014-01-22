/*
 The Sensor Net Arduino library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>
	
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#ifndef Relay_h
#define Relay_h

#include "Sensor.h"

#ifdef DEBUG
#define debug(x,...) debugPrint(x, ##__VA_ARGS__)
#else
#define debug(x,...)
#endif



#define EEPROM_ROUTES_ADDRESS ((uint8_t)3) // Where to start storing routing information in EEPROM. Will allocate 256 bytes.


class Relay : public Sensor
{

	public:
	   /**
		* Constructor
		*
		* Creates a new instance of Relay class.
		*
		* @param _cepin The pin attached to RF24 Chip Enable on the RF module
		* @param _cspin The pin attached to RF24 Chip Select
		*/
		Relay(uint8_t _cepin, uint8_t _cspin);

		void begin(uint8_t _radioId);
		boolean messageAvailable();
		boolean send(message_s message, int length);

	protected:

	private:
		uint8_t childNodeTable[256]; // Buffer to store child node information. Store this in EEPROM

		uint8_t getChildRoute(uint8_t childId);
		void addChildRoute(uint8_t childId, uint8_t route);
		void removeChildRoute(uint8_t childId);
		void clearChildRoutes();
		void sendChildren();
		void relayMessage(uint8_t length, uint8_t pipe);

};

#endif
