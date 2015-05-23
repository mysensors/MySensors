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

#include "MyGateway.h"

// Inline function and macros
static inline MyMessage& build(MyMessage &msg, uint8_t type) {
	msg.sender = GATEWAY_ADDRESS;
	msg.destination = GATEWAY_ADDRESS;
	msg.sensor = 0;
	msg.type = type;
	mSetCommand(msg, C_INTERNAL);
	mSetRequestAck(msg, false);
	mSetAck(msg, false);
	return msg;
}

MyGateway::MyGateway(MyGatewayTransport &_transport, MyTransport &_radio,
		MyHw &_hw
#ifdef MY_SIGNING_FEATURE
		, MySigning &_signer
#endif
#ifdef MY_LEDS_BLINKING_FEATURE
		, uint8_t _rx, uint8_t _tx, uint8_t _er, unsigned long _blink_period
#endif
		) :
		transport(_transport), MySensor(_radio, _hw
#ifdef MY_SIGNING_FEATURE
		, _signer
#endif
#ifdef MY_LEDS_BLINKING_FEATURE
		, _rx, _tx, _er, _blink_period
#endif
		)
{
#ifdef MY_INCLUSION_MODE_FEATURE
	inclusionMode = false;
#endif
}

void MyGateway::begin(void (*_msgCallback)(const MyMessage &)
#ifdef MY_INCLUSION_MODE_FEATURE
				, int _inclusionModeDuration
#ifdef MY_INCLUSION_BUTTON_FEATURE
				, uint8_t _inclusionModeButtonPin
#endif
#endif

) {
	// Setup digital in that triggers inclusion mode
#ifdef MY_INCLUSION_MODE_FEATURE
	inclusionDuration = _inclusionModeDuration*1000;
#ifdef MY_INCLUSION_BUTTON_FEATURE
	inclusionButtonPin = _inclusionModeButtonPin;
	pinMode(inclusionButtonPin, INPUT);
	digitalWrite(inclusionButtonPin, HIGH);
#endif
#endif

	// initialize the transport driver
	if (!transport.begin()) {
		debug(PSTR("transport driver init fail\n"));
		while(1); // Nothing more we can do
	}

	// Start MySensors in repeater mode
	MySensor::begin(_msgCallback, GATEWAY_ADDRESS, true, GATEWAY_ADDRESS);

	// Startup complete
	transport.send(build(msg, I_GATEWAY_READY).set("Gateway startup complete."));

}

boolean MyGateway::sendRoute(MyMessage &message) {
	if (message.destination == GATEWAY_ADDRESS) {
		// This is a message sent from a sensor attached on the gateway node.
		// Pass it directly to the gateway transport layer.
		transport.send(message);
	} else {
		MySensor::sendRoute(message);
	}
}

boolean MyGateway::process() {
#ifdef MY_INCLUSION_MODE_FEATURE
	checkInclusionMode();
#endif
	bool newMessage = false;
	if (transport.available()) {
		MyMessage &gmsg = transport.receive();
		if (msg.destination == GATEWAY_ADDRESS) {
			// Check if sender requests an ack back.
			if (mGetRequestAck(gmsg)) {
				// Copy message
				tmpMsg = gmsg;
				mSetRequestAck(tmpMsg, false); // Reply without ack flag (otherwise we would end up in an eternal loop)
				mSetAck(tmpMsg, true);
				tmpMsg.sender = nc.nodeId;
				tmpMsg.destination = msg.sender;
				transport.send(tmpMsg);
			}
			if (mGetCommand(gmsg) == C_INTERNAL) {
				if (msg.type == I_VERSION) {
					// Request for version. Create the response
					transport.send(build(msg, I_VERSION).set(LIBRARY_VERSION));
#ifdef MY_INCLUSION_MODE_FEATURE
				} else if (msg.type == I_INCLUSION_MODE) {
					// Request to change inclusion mode
					setInclusionMode(atoi(msg.data) == 1);
#endif
				} else {
					processInternalMessages(gmsg);
				}
			} else {
				// Call incoming message callback if available
				if (msgCallback != NULL) {
					msgCallback(msg);
				}
				newMessage = true;
			}
		}
	}
	return MySensor::process() || newMessage;
}

#ifdef MY_INCLUSION_MODE_FEATURE
void MyGateway::setInclusionMode(boolean newMode) {
  if (newMode != inclusionMode) {
    inclusionMode = newMode;
    // Send back mode change to controller
    transport.send(build(msg, I_INCLUSION_MODE).set(inclusionMode?1:0));
    if (inclusionMode) {

    	inclusionStartTime = hw_millis();
    	Serial.println(inclusionStartTime);
    	Serial.println(inclusionDuration);

    }
  }
}

void MyGateway::checkInclusionMode() {

	#ifdef MY_INCLUSION_BUTTON_FEATURE
	if (!inclusionMode && digitalRead(inclusionButtonPin) == LOW) {
		// Start inclusion mode
		setInclusionMode(true);
	}
	#endif

	if (inclusionMode && hw_millis()-inclusionStartTime>inclusionDuration) {
		// inclusionTimeInMinutes minute(s) has passed.. stop inclusion mode
		setInclusionMode(false);
	}
}


#endif


