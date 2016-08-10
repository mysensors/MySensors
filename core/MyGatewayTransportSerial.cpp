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


#include "MyConfig.h"
#include "MyProtocol.h"
#include "MyGatewayTransport.h"
#include "MyMessage.h"
#include "MyProtocol.h"

 // global variables
extern MyMessage _msgTmp;

char _serialInputString[MY_GATEWAY_MAX_RECEIVE_LENGTH];    // A buffer for incoming commands from serial interface
int _serialInputPos;
MyMessage _serialMsg;


bool gatewayTransportSend(MyMessage &message) {
    setIndication(INDICATION_GW_TX);
	MY_SERIALDEVICE.print(protocolFormat(message));
	// Serial print is always successful
	return true;
}

bool gatewayTransportInit() {
	gatewayTransportSend(buildGw(_msgTmp, I_GATEWAY_READY).set("Gateway startup complete."));
	// Send presentation of locally attached sensors (and node if applicable)
	presentNode();

	return true;
}


bool gatewayTransportAvailable() {
	while (MY_SERIALDEVICE.available()) {
		// get the new byte:
		char inChar = (char) MY_SERIALDEVICE.read();
		// if the incoming character is a newline, set a flag
		// so the main loop can do something about it:
		if (_serialInputPos < MY_GATEWAY_MAX_RECEIVE_LENGTH - 1) {
			if (inChar == '\n') {
				_serialInputString[_serialInputPos] = 0;
				bool ok = protocolParse(_serialMsg, _serialInputString);
				_serialInputPos = 0;
				return ok;
			} else {
				// add it to the inputString:
				_serialInputString[_serialInputPos] = inChar;
				_serialInputPos++;
			}
		} else {
			// Incoming message too long. Throw away
			_serialInputPos = 0;
		}
	}
	return false;
}

MyMessage & gatewayTransportReceive() {
	// Return the last parsed message
	return _serialMsg;
}
