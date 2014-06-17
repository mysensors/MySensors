/*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#include "MyGateway.h"


MyGateway::MyGateway(uint8_t _cepin, uint8_t _cspin, uint8_t _inclusion_time) : MySensor(_cepin, _cspin) {
	ledMode = false;
	inclusionTime = _inclusion_time;
}

MyGateway::MyGateway(uint8_t _cepin, uint8_t _cspin, uint8_t _inclusion_time, uint8_t _inclusion_pin, uint8_t _rx, uint8_t _tx, uint8_t _er) : MySensor(_cepin, _cspin) {
	ledMode = true;
	pinInclusion = _inclusion_pin;
	inclusionTime = _inclusion_time;
	pinRx = _rx;
	pinTx = _tx;
	pinEr = _er;
}

void MyGateway::begin(rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate, void (*inDataCallback)(char *)) {
	Serial.begin(BAUD_RATE);
	repeaterMode = true;
	isGateway = true;
	setupRepeaterMode();

	if (inDataCallback != NULL) {
		useWriteCallback = true;
		dataCallback = inDataCallback;
	} else {
		useWriteCallback = false;
	}

	nc.nodeId = 0;
	nc.distance = 0;
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
	RF24::openReadingPipe(WRITE_PIPE, BASE_RADIO_ID);
	RF24::openReadingPipe(CURRENT_NODE_PIPE, BASE_RADIO_ID);
	RF24::startListening();

	// Send startup log message on serial
	serial(PSTR("0;0;%d;%d;Arduino startup complete.\n"),  C_INTERNAL, I_LOG_MESSAGE);

}


boolean MyGateway::isLedMode() {
	return ledMode;
}

void MyGateway::startInclusionInterrupt() {
	  buttonTriggeredInclusion = true;
}


void MyGateway::checkButtonTriggeredInclusion() {
   if (buttonTriggeredInclusion) {
    // Ok, someone pressed the inclusion button on the gateway
    // start inclusion mode for 1 munute.
    serial(PSTR("0;0;%d;%d;Inclusion started by button.\n"),  C_INTERNAL, I_LOG_MESSAGE);
    buttonTriggeredInclusion = false;
    setInclusionMode(true);
  }
}

void MyGateway::checkInclusionFinished() {
	if (inclusionMode && millis()-inclusionStartTime>60000UL*inclusionTime) {
	     // inclusionTimeInMinutes minute(s) has passed.. stop inclusion mode
	    setInclusionMode(false);
	 }
}


void MyGateway::parseAndSend(char *commandBuffer) {
  boolean ok = false;
  char *str, *p, *value=NULL;
  int i = 0;
  uint16_t destination = 0;
  uint8_t sensor = 0;
  uint8_t command = 0;
  uint8_t type = 0;
  uint8_t ack = 0;

  // Extract command data coming on serial line
  for (str = strtok_r(commandBuffer, ";", &p);       // split using semicolon
  		str && i < 6;         // loop while str is not null an max 5 times
  		str = strtok_r(NULL, ";", &p)               // get subsequent tokens
				) {
	switch (i) {
	  case 0: // Radioid (destination)
	 	destination = atoi(str);
		break;
	  case 1: // Childid
		sensor = atoi(str);
		break;
	  case 2: // Message type
		command = atoi(str);
		break;
	  case 3: // Should we request ack from destination?
		ack = atoi(str);
		break;
	  case 4: // Data type
		type = atoi(str);
		break;
	  case 5: // Variable value
		value = str;
		break;
	  }
	  i++;
  }

  if (destination==GATEWAY_ADDRESS && command==C_INTERNAL) {
    // Handle messages directed to gateway
    if (type == I_VERSION) {
      // Request for version
      serial(PSTR("0;0;%d;%d;%s\n"),C_INTERNAL, I_VERSION, LIBRARY_VERSION);
    } else if (type == I_INCLUSION_MODE) {
      // Request to change inclusion mode
      setInclusionMode(atoi(value) == 1);
    }
  } else {
    txBlink(1);
    msg.sender = GATEWAY_ADDRESS;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetAck(msg,ack==0?false:true);
	msg.set(value);
    ok = sendRoute(msg);
    if (!ok) {
      errBlink(1);
    }
  }
}


void MyGateway::setInclusionMode(boolean newMode) {
  if (newMode != inclusionMode)
    inclusionMode = newMode;
    // Send back mode change on serial line to ack command
    serial(PSTR("0;0;%d;%d;%d\n"), C_INTERNAL, I_INCLUSION_MODE, inclusionMode?1:0);

    if (inclusionMode) {
      inclusionStartTime = millis();
    }
}


// Override normal validate to add error blink if crc check fails
uint8_t MyGateway::validate(MyMessage &message) {
	uint8_t res = MySensor::validate(message);
	if (res == VALIDATE_BAD_CRC) {
		errBlink(1);
	}
	return res;
}

void MyGateway::processRadioMessage() {
	if (process()) {
	  // A new message was received from one of the sensors
	  MyMessage message = getLastMessage();
	  if (mGetCommand(message) == C_PRESENTATION && inclusionMode) {
		rxBlink(3);
	  } else {
		rxBlink(1);
	  }
	  // Pass along the message from sensors to serial line
	  serial(message);
	}

	checkButtonTriggeredInclusion();
	checkInclusionFinished();
}

void MyGateway::serial(const char *fmt, ... ) {
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

void MyGateway::serial(MyMessage &msg) {
  serial(PSTR("%d;%d;%d;%d;%s\n"),msg.sender, msg.sensor, mGetCommand(msg), msg.type, msg.getString(convBuf));
}


void MyGateway::ledTimersInterrupt() {
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

void MyGateway::rxBlink(uint8_t cnt) {
  if(countRx == 255) { countRx = cnt; }
}
void MyGateway::txBlink(uint8_t cnt) {
  if(countTx == 255 && !inclusionMode) { countTx = cnt; }
}
void MyGateway::errBlink(uint8_t cnt) {
  if(countErr == 255) { countErr = cnt; }
}

