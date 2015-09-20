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

#include "MyTransport.h"
#include "MyTransportRFM69.h"

MyTransportRFM69::MyTransportRFM69(uint8_t freqBand, uint8_t networkId, uint8_t slaveSelectPin, uint8_t interruptPin, bool isRFM69HW, uint8_t interruptNum)
	:
	MyTransport(),
	radio(slaveSelectPin, interruptPin, isRFM69HW, interruptNum),
	_freqBand(freqBand),
	_networkId(networkId)
{
}

bool MyTransportRFM69::init() {
	// Start up the radio library (_address will be set later by the MySensors library)
	radio.initialize(_freqBand, _address, _networkId);
#ifdef RFM69_ENABLE_ENCRYPTION
    radio.encrypt(RFM69_ENCRYPTKEY);
#endif
	return true;
}

void MyTransportRFM69::setAddress(uint8_t address) {
	_address = address;
	radio.setAddress(address);
}

uint8_t MyTransportRFM69::getAddress() {
	return _address;
}

bool MyTransportRFM69::send(uint8_t to, const void* data, uint8_t len) {
	return radio.sendWithRetry(to,data,len);
}

bool MyTransportRFM69::available(uint8_t *to) {
	if (radio.TARGETID == BROADCAST_ADDRESS)
		*to = BROADCAST_ADDRESS;
	else
		*to = _address;
	return radio.receiveDone();
}

uint8_t MyTransportRFM69::receive(void* data) {
	memcpy(data,(const void *)radio.DATA, radio.DATALEN);
	// Send ack back if this message wasn't a broadcast
	if (radio.TARGETID != RF69_BROADCAST_ADDR)
		radio.ACKRequested();
    radio.sendACK();
	return radio.DATALEN;
}	

void MyTransportRFM69::powerDown() {
	radio.sleep();
}
