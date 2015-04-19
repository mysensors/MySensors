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

#include <JeeLib.h>
#include "MyTransport.h"
#include "MyTransportRFM12.h"

MyTransportRFM12::MyTransportRFM12(uint8_t freqBand, uint8_t networkId)
	:
	MyTransport(),
	_freqBand(freqBand),
	_networkId(networkId)
{
}

void MyTransportRFM12::init() {
	// Start up the radio library (_address will be set later by the MySensors library)
   rf12_initialize(_address, _freqBand, _networkId);
   
#ifdef RFM12_ENABLE_ENCRYPTION
    // radio.Encrypt(ENCRYPTKEY);
#endif
	
}

void MyTransportRFM12::setAddress(uint8_t address) {
   if(address == 0) 
      address = RFM12_BROADCAST_ADDRESS; // set adress to jeelib broadcast addr (31)
	_address = address;
   rf12_initialize(_address, _freqBand, _networkId);
}

uint8_t MyTransportRFM12::getAddress() {
   //if(_address == 0) 
   //   return RFM12_BROADCAST_ADDRESS; // set adress to jeelib broadcast addr (31)
	return _address;
}

bool MyTransportRFM12::send(uint8_t to, const void* data, uint8_t len) {

   rf12_sendStart(to, data, len);

   return true;
}

bool MyTransportRFM12::available(uint8_t *to) {
   if ((uint8_t)rf12_data[0] == BROADCAST_ADDRESS){
		*to = BROADCAST_ADDRESS;
	} 
   else {
      *to = _address;
   }
   return rf12_recvDone();
}

uint8_t MyTransportRFM12::receive(void* data) {
   memcpy(data,(const void *)rf12_data, (unsigned int)rf12_len);

	if (RF12_WANTS_ACK && (uint8_t)rf12_data[0] != RFM12_BROADCAST_ADDRESS){
      rf12_sendStart(RF12_ACK_REPLY, 0, 0);
   }

	return rf12_len;
}	

void MyTransportRFM12::powerDown() {
	rf12_sleep(RF12_SLEEP);
}
