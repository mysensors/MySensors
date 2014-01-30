/*
 The Sensor Net Arduino library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>
	
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#ifndef Gateway_h
#define Gateway_h

#include "Relay.h"


#define MAX_RECEIVE_LENGTH 50 // Max buffersize needed for messages coming from vera
#define MAX_SEND_LENGTH 120 // Max buffersize needed for messages coming from vera


class Gateway : public Relay
{
	public:
		/**
		* Constructor
		*
		* Creates a new instance of Gateway class.
		* If you don't use status leds and/or inclusion mode button on your arduino gateway
		* you can disable this functionality by calling the 2 argument constructor.
		*
		* @param _cepin The pin attached to RF24 Chip Enable on the RF module
		* @param _cspin The pin attached to RF24 Chip Select
		* @param _inclusion Digital pin that triggers inclusion mode
		* @param _inclusion_time Time of inclusion mode (in minutes)
		* @param _rx Digital pin for receive led
		* @param _tx Digital pin for transfer led
		* @param _er Digital pin for error led
		*
		*/
		Gateway(uint8_t _cepin, uint8_t _cspin, uint8_t _inclusion_time);
		Gateway(uint8_t _cepin, uint8_t _cspin, uint8_t _inclusion, uint8_t _inclusion_time, uint8_t _rx, uint8_t _tx, uint8_t _er);

		void begin(uint8_t _radioId = 0);
		void processRadioMessage();
	    void parseAndSend(String inputString);
	    boolean isLedMode();
	    void ledTimersInterrupt();
	    void startInclusionInterrupt();



	private:
	    char commandBuffer[MAX_RECEIVE_LENGTH];
	    char serialBuffer[MAX_SEND_LENGTH]; // Buffer for building string when sending data to vera
	    unsigned long inclusionStartTime;
	    boolean inclusionMode; // Keeps track on inclusion mode
	    boolean buttonTriggeredInclusion;
	    volatile uint8_t countRx;
	    volatile uint8_t countTx;
	    volatile uint8_t countErr;
	    boolean ledMode;
	    uint8_t pinInclusion;
	    uint8_t inclusionTime;
	    uint8_t pinRx;
	    uint8_t pinTx;
	    uint8_t pinEr;

	    void serial(const char *fmt, ... );
	    uint8_t validate();
	    void serial(message_s msg);
	    void interruptStartInclusion();
	    void checkButtonTriggeredInclusion();
	    void setInclusionMode(boolean newMode);
	    void checkInclusionFinished();
	    void ledTimers();
	    void rxBlink(uint8_t cnt);
	    void txBlink(uint8_t cnt);
	    void errBlink(uint8_t cnt);
};



#endif
