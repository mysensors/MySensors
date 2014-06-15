/*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>
	
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#ifndef MyGateway_h
#define MyGateway_h

#include "MySensor.h"

#define MAX_RECEIVE_LENGTH 100 // Max buffersize needed for messages coming from vera
#define MAX_SEND_LENGTH 120 // Max buffersize needed for messages coming from vera

class MyGateway : public MySensor
{
	public:
		/**
		* Constructor
		*
		* Creates a new instance of MyGateway class.
		* If you don't use status leds and/or inclusion mode button on your arduino gateway
		* you can disable this functionality by calling the 2 argument constructor.
		*
		* @param _cepin The pin attached to RF24 Chip Enable on the RF module (default 9)
		* @param _cspin The pin attached to RF24 Chip Select (defualt 10)
		* @param _inclusion_time Time of inclusion mode (in minutes, default 1)
		* @param _inclusion_pin Digital pin that triggers inclusion mode
		* @param _rx Digital pin for receive led
		* @param _tx Digital pin for transfer led
		* @param _er Digital pin for error led
		*
		*/
		MyGateway(uint8_t _cepin=9, uint8_t _cspin=10, uint8_t _inclusion_time = 1);
		MyGateway(uint8_t _cepin, uint8_t _cspin, uint8_t _inclusion_time, uint8_t _inclusion_pin, uint8_t _rx, uint8_t _tx, uint8_t _er);

		/* Use this and pass a function that should be called when you want to process commands that arrive from radio network */
		void begin(rf24_pa_dbm_e paLevel=RF24_PA_LEVEL_GW, uint8_t channel=RF24_CHANNEL, rf24_datarate_e dataRate=RF24_DATARATE, void (*dataCallback)(char *)=NULL);

		void processRadioMessage();
	    void parseAndSend(char *inputString);
	    boolean isLedMode();
	    void ledTimersInterrupt();
	    void startInclusionInterrupt();

	private:
	    char convBuf[MAX_PAYLOAD*2+1];
	    char serialBuffer[MAX_SEND_LENGTH]; // Buffer for building string when sending data to vera
	    unsigned long inclusionStartTime;
	    boolean inclusionMode; // Keeps track on inclusion mode
	    boolean buttonTriggeredInclusion;
	    volatile uint8_t countRx;
	    volatile uint8_t countTx;
	    volatile uint8_t countErr;
	    boolean ledMode;
	    boolean useWriteCallback;
	    void (*dataCallback)(char *);


	    uint8_t pinInclusion;
	    uint8_t inclusionTime;
	    uint8_t pinRx;
	    uint8_t pinTx;
	    uint8_t pinEr;

	    void serial(const char *fmt, ... );
	    void serial(MyMessage &msg);
	    uint8_t validate(MyMessage &message);
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
