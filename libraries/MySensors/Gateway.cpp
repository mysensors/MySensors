/*
 The Sensor Net Arduino library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#include "Gateway.h"

Gateway::Gateway(uint8_t _cepin, uint8_t _cspin, uint8_t _inclusion_time) : Relay(_cepin, _cspin) {
	ledMode = false;
	isRelay = true;
	inclusionTime = _inclusion_time;
}

Gateway::Gateway(uint8_t _cepin, uint8_t _cspin, uint8_t _inclusion_time, uint8_t _inclusion_pin, uint8_t _rx, uint8_t _tx, uint8_t _er) : Relay(_cepin, _cspin) {
	ledMode = true;
	isRelay = true;
	pinInclusion = _inclusion_pin;
	inclusionTime = _inclusion_time;
	pinRx = _rx;
	pinTx = _tx;
	pinEr = _er;
}

void Gateway::begin(rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate, void (*inDataCallback)(char *)) {
	Serial.begin(BAUD_RATE);

	if (inDataCallback != NULL) {
		useWriteCallback = true;
		dataCallback = inDataCallback;
	} else {
		useWriteCallback = false;
	}

	radioId = 0;
	distance = 0;
	inclusionMode = 0;
	buttonTriggeredInclusion = false;
	countRx = 0;
	countTx = 0;
	countErr = 0;

	if (ledMode) {
		// Setup led pins
		pinMode(pinRx, OUTPUT);
		pinMode(pinTx, OUTPUT);
		pinMode(pinEr, OUTPUT);
		digitalWrite(pinRx, LOW);
		digitalWrite(pinTx, LOW);
		digitalWrite(pinEr, LOW);

		// Setup digital in that triggers inclusion mode
		pinMode(pinInclusion, INPUT);
		digitalWrite(pinInclusion, HIGH);

		// Set initial state of leds
		digitalWrite(pinRx, HIGH);
		digitalWrite(pinTx, HIGH);
		digitalWrite(pinEr, HIGH);

	}

	// Start up the radio library
	setupRadio(paLevel, channel, dataRate);
	RF24::openReadingPipe(CURRENT_NODE_PIPE, BASE_RADIO_ID);
	RF24::startListening();

	// Send startup log message on serial
	serial(PSTR("0;0;%d;%d;Arduino startup complete.\n"),  M_INTERNAL, I_LOG_MESSAGE);

}


boolean Gateway::isLedMode() {
	return ledMode;
}

void Gateway::startInclusionInterrupt() {
	  buttonTriggeredInclusion = true;
}


void Gateway::checkButtonTriggeredInclusion() {
   if (buttonTriggeredInclusion) {
    // Ok, someone pressed the inclusion button on the gateway
    // start inclusion mode for 1 munute.
    serial(PSTR("0;0;%d;%d;Inclusion started by button.\n"),  M_INTERNAL, I_LOG_MESSAGE);
    buttonTriggeredInclusion = false;
    setInclusionMode(true);

#ifdef DEBUG
    sendChildren();
#endif

  }
}

void Gateway::checkInclusionFinished() {
	if (inclusionMode && millis()-inclusionStartTime>60000UL*inclusionTime) {
	     // inclusionTimeInMinutes minute(s) has passed.. stop inclusion mode
	    setInclusionMode(false);
	 }
}


void Gateway::parseAndSend(char *commandBuffer) {
  boolean ok = false;
  char *str, *p, *value=NULL;
  int i = 0;
  uint16_t sensorRadioId = 0;
  uint8_t childId = 0;
  uint8_t messageType = 0;
  uint8_t type = 0;

  // Extract command data coming on serial line
  for (str = strtok_r(commandBuffer, ";", &p);       // split using semicolon
  		str && i < 5;         // loop while str is not null an max 5 times
  		str = strtok_r(NULL, ";", &p)               // get subsequent tokens
				) {
	switch (i) {
	  case 0: // Radioid (destination)
	 	sensorRadioId = atoi(str);
		break;
	  case 1: // Childid
		childId = atoi(str);
		break;
	  case 2: // Message type
		messageType = atoi(str);
		break;
	  case 3: // Data type
		type = atoi(str);
		break;
	  case 4: // Variable value
		value = str;
		break;
	  }
	  i++;
  }

  if (sensorRadioId==GATEWAY_ADDRESS && messageType==M_INTERNAL) {
    // Handle messages directed to gateway
    if (type == I_VERSION) {
      // Request for version
      serial(PSTR("0;0;%d;%d;%s\n"),M_INTERNAL, I_VERSION, LIBRARY_VERSION);
    } else if (type == I_INCLUSION_MODE) {
      // Request to change inclusion mode
      setInclusionMode(atoi(value) == 1);
    }
  } else {
    txBlink(1);

    ok = sendData(GATEWAY_ADDRESS, sensorRadioId, childId, messageType, type, value, strlen(value), false);
    if (!ok) {
      errBlink(1);
    }
  }
}


void Gateway::setInclusionMode(boolean newMode) {
  if (newMode != inclusionMode)
    inclusionMode = newMode;
    // Send back mode change on serial line to ack command
    serial(PSTR("0;0;%d;%d;%d\n"), M_INTERNAL, I_INCLUSION_MODE, inclusionMode?1:0);

    if (inclusionMode) {
      inclusionStartTime = millis();
    }
}


// Override normal validate to add error blink if crc check fails
uint8_t Gateway::validate(uint8_t length) {
	uint8_t res = Sensor::validate(length);
	if (res == VALIDATE_BAD_CRC) {
		errBlink(1);
	}
	return res;
}

void Gateway::processRadioMessage() {
	if (messageAvailable()) {
	  // A new message was received from one of the sensors
	  message_s msg = getMessage();
	  if (msg.header.messageType == M_PRESENTATION && inclusionMode) {
		rxBlink(3);
	  } else {
		rxBlink(1);
	  }
	  // Pass along the message from sensors to serial line
	  serial(msg);
	}

	checkButtonTriggeredInclusion();
	checkInclusionFinished();
}

void Gateway::serial(const char *fmt, ... ) {
   va_list args;
   va_start (args, fmt );
   vsnprintf_P(serialBuffer, MAX_SEND_LENGTH, fmt, args);
   va_end (args);
   Serial.print(serialBuffer);
   if (useWriteCallback) {
	   // We have a registered write callback (probably Ethernet)
	   dataCallback(serialBuffer);
   }
}

void Gateway::serial(message_s msg) {
  serial(PSTR("%d;%d;%d;%d;%s\n"),msg.header.from, msg.header.childId, msg.header.messageType, msg.header.type, msg.data);
}


void Gateway::ledTimersInterrupt() {
  if(countRx && countRx != 255) {
    // switch led on
    digitalWrite(pinRx, LOW);
  } else if(!countRx) {
     // switching off
     digitalWrite(pinRx, HIGH);
   }
   if(countRx != 255) { countRx--; }

  if(countTx && countTx != 255) {
    // switch led on
    digitalWrite(pinTx, LOW);
  } else if(!countTx) {
     // switching off
     digitalWrite(pinTx, HIGH);
   }
   if(countTx != 255) { countTx--; }
   else if(inclusionMode) { countTx = 8; }

  if(countErr && countErr != 255) {
    // switch led on
    digitalWrite(pinEr, LOW);
  } else if(!countErr) {
     // switching off
     digitalWrite(pinEr, HIGH);
   }
   if(countErr != 255) { countErr--; }

}

void Gateway::rxBlink(uint8_t cnt) {
  if(countRx == 255) { countRx = cnt; }
}
void Gateway::txBlink(uint8_t cnt) {
  if(countTx == 255 && !inclusionMode) { countTx = cnt; }
}
void Gateway::errBlink(uint8_t cnt) {
  if(countErr == 255) { countErr = cnt; }
}

