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

#ifndef MyTransportRF12_h
#define MyTransportRF12_h

#include "MyConfig.h"
#include "MyTransport.h"
#include <stdint.h>
#include "utility/RFM12B.h"
#include <SPI.h>

#define RF12_SPI_CS SPI_SS
#define RF12_IRQ_PIN RFM_IRQ
#define RF12_IRQ_NUM 0


class MyTransportRFM12 : public MyTransport
{ 
public:
	MyTransportRFM12(uint8_t freqBand=RFM12_FREQUENCY, uint8_t networkId=RFM12_NETWORKID, uint8_t slaveSelectPin=RF12_SPI_CS, uint8_t interruptPin=RF12_IRQ_PIN, bool isRFM12HW=false, uint8_t interruptNum=RF12_IRQ_NUM);
	void init();
	void setAddress(uint8_t address);
	uint8_t getAddress();
	bool send(uint8_t to, const void* data, uint8_t len);
	bool available(uint8_t *to);
	uint8_t receive(void* data);
	void powerDown();
private:
	RFM12B radio;
	uint8_t _address;
	uint8_t _freqBand;
	uint8_t _networkId;
};

#endif
