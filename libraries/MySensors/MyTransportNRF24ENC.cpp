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
#include "MyTransportNRF24ENC.h"

//AES aes;


MyTransportNRF24ENC::MyTransportNRF24ENC(uint8_t ce, uint8_t cs, uint8_t paLevel, uint8_t channel, rf24_datarate_e datarate)
	:
	MyTransport(),
	rf24(ce, cs),
	_paLevel(paLevel),
	_channel(channel),
	_datarate(datarate)
{
}

bool MyTransportNRF24ENC::init() {
	// Start up the radio library

	//randomseed(analogRead(RNDPIN));
	uint8_t psk[] = { PSK };

	aes.set_key(psk, 16);

	rf24.begin();

	if (!rf24.isPVariant()) {
		return false;
	}
	rf24.setAutoAck(1);
	rf24.setAutoAck(BROADCAST_PIPE,false); // Turn off auto ack for broadcast
	rf24.enableAckPayload();
	rf24.setChannel(_channel);
	rf24.setPALevel(_paLevel);
	rf24.setDataRate(_datarate);
	rf24.setRetries(5,15);
	rf24.setCRCLength(RF24_CRC_16);
	rf24.enableDynamicPayloads();

	// All nodes listen to broadcast pipe (for FIND_PARENT_RESPONSE messages)
	rf24.openReadingPipe(BROADCAST_PIPE, TO_ADDR(BROADCAST_ADDRESS));
	return true;
}

void MyTransportNRF24ENC::setAddress(uint8_t address) {
	_address = address;
	rf24.openReadingPipe(WRITE_PIPE, TO_ADDR(address));
	rf24.openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(address));
	rf24.startListening();
}

uint8_t MyTransportNRF24ENC::getAddress() {
	return _address;
}

bool MyTransportNRF24ENC::send(uint8_t to, const void* data, uint8_t len) {
	// Make sure radio has powered up
	//encrypt

	uint8_t dataenc[32] = {0} ;
	memcpy(dataenc,data,len);

	len=len > 16 ? 32 : 16;
	aes.cbc_encrypt(dataenc, dataenc, len/16);
	
	rf24.powerUp();
	rf24.stopListening();
	rf24.openWritingPipe(TO_ADDR(to));
	bool ok = rf24.write(dataenc, len, to == BROADCAST_ADDRESS);
	rf24.startListening();
	return ok;
}

bool MyTransportNRF24ENC::available(uint8_t *to) {
	uint8_t pipe = 255;
	boolean avail = rf24.available(&pipe);
	(void)avail; //until somebody makes use of 'avail'
	if (pipe == CURRENT_NODE_PIPE)
		*to = _address;
	else if (pipe == BROADCAST_PIPE)
		*to = BROADCAST_ADDRESS;
	return (rf24.available() && pipe < 6);
}

uint8_t MyTransportNRF24ENC::receive(void* data) {
	uint8_t len = rf24.getDynamicPayloadSize();
	rf24.read(data, len);
	//decrypt
	aes.cbc_decrypt((byte*)(data), (byte*)(data), len>16?2:1);
	return len;
}

void MyTransportNRF24ENC::powerDown() {
	rf24.powerDown();
}
