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
#include "MyTransportRFM12.h"

MyTransportRFM12::MyTransportRFM12(uint8_t freqBand, uint8_t networkId, uint8_t slaveSelectPin, uint8_t interruptPin, bool isRFM12HW, uint8_t interruptNum)
	:
	MyTransport(),
	radio(),
	_freqBand(freqBand),
	_networkId(networkId)
{
}

void MyTransportRFM12::init() {
	// Start up the radio library (_address will be set later by the MySensors library)
	radio.Initialize(_address, _freqBand, _networkId);
#ifdef RFM12_ENABLE_ENCRYPTION
    radio.Encrypt(ENCRYPTKEY);
#endif
	
}

void MyTransportRFM12::setAddress(uint8_t address) {
	_address = address;
	//radio.setAddress(address);
}

uint8_t MyTransportRFM12::getAddress() {
	return _address;
}

bool MyTransportRFM12::send(uint8_t to, const void* data, uint8_t len) {
   radio.Send(to, data, len, true);
}

bool MyTransportRFM12::available(uint8_t *to) {
	if (radio.GetSender() == BROADCAST_ADDRESS)
		*to = BROADCAST_ADDRESS;
	else
		*to = _address;
	return radio.ReceiveComplete();
}

uint8_t MyTransportRFM12::receive(void* data) {
	memcpy(data,(const void *)radio.Data, (unsigned int)radio.DataLen);
	// Send ack back if this message wasn't a broadcast
	if (radio.GetSender() != BROADCAST_ADDRESS)
		radio.ACKRequested();
    radio.SendACK();
   volatile uint8_t len = (volatile uint8_t)*radio.DataLen;
	return len;
}	

void MyTransportRFM12::powerDown() {
	radio.Sleep();
}
