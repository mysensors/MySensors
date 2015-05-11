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
static inline MyMessage& build (MyMessage &msg, uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.sender = sender;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}

MyGateway::MyGateway(MyGatewayTransport &_transport,MyTransport &_radio, MyHw &_hw
#ifdef MY_SIGNING_FEATURE
	, MySigning &_signer
#endif
#ifdef WITH_LEDS_BLINKING
	, uint8_t _rx, uint8_t _tx, uint8_t _er, unsigned long _blink_period
#endif
		) : transport(_transport),MySensor(_radio, _hw
#ifdef MY_SIGNING_FEATURE
	, _signer
#endif
#ifdef WITH_LEDS_BLINKING
	, _rx, _tx, _er, _blink_period
#endif
		)
{

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
	bool newMessage = false;
	if (transport.available()) {
		MyMessage &gmsg = transport.receive();
		if (msg.destination == GATEWAY_ADDRESS) {
			// Check if sender requests an ack back.
			if (mGetRequestAck(gmsg)) {
				// Copy message
				tmpMsg = gmsg;
				mSetRequestAck(tmpMsg,false); // Reply without ack flag (otherwise we would end up in an eternal loop)
				mSetAck(tmpMsg,true);
				tmpMsg.sender = nc.nodeId;
				tmpMsg.destination = msg.sender;
				transport.send(tmpMsg);
			}
			if (mGetCommand(gmsg) == C_INTERNAL) {
				if (msg.type == I_VERSION) {
					// Request for version. Create the response
					transport.send(build(msg, GATEWAY_ADDRESS, GATEWAY_ADDRESS, GATEWAY_ADDRESS, C_INTERNAL, I_VERSION, false).set(LIBRARY_VERSION));
				} else if (msg.type == I_INCLUSION_MODE) {
					// Request to change inclusion mode
					//setInclusionMode(atoi(msg.data) == 1);
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
