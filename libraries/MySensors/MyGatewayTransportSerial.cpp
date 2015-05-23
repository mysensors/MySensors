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
 */

#include "MyGatewayTransportSerial.h"


MyGatewayTransportSerial::MyGatewayTransportSerial(MyProtocol &_protocol) : MyGatewayTransport(_protocol),inputPos(0)
{

}

bool MyGatewayTransportSerial::begin() {
	return true;
}

bool MyGatewayTransportSerial::send(MyMessage &message) {
	Serial.print(protocol.format(message));
	// Serial print is always successful
	return true;
}

bool MyGatewayTransportSerial::available() {
	bool available = false;
	while (Serial.available()) {
		// get the new byte:
		char inChar = (char) Serial.read();
		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if (inputPos < MAX_RECEIVE_LENGTH - 1 && !available) {
			if (inChar == '\n') {
				inputString[inputPos] = 0;
				available = true;
			} else {
				// add it to the inputString:
				inputString[inputPos] = inChar;
				inputPos++;
			}
		} else {
			// Incoming message too long. Throw away
			inputPos = 0;
		}
	}
	if (available) {
		// Parse message and return parse result
		available = protocol.parse(msg, inputString);
		inputPos = 0;
	}
	return available;
}

MyMessage & MyGatewayTransportSerial::receive() {
	// Return the last parsed message
	return msg;
}
