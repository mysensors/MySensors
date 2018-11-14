/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2018 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * -------------------------------------------------------------------------------
 * 
 */

PJON<SoftwareBitBang> bus;

char _data[MAX_MESSAGE_LENGTH];
uint8_t _packet_len;
unsigned char _packet_from;
bool _packet_received;

// debug
#if defined(MY_DEBUG_VERBOSE_PJON)
#define PJON_DEBUG(x,...)	DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< Debug print
#else
#define PJON_DEBUG(x,...)	//!< DEBUG null
#endif

bool transportSend(const uint8_t to, const void* data, const uint8_t length, const bool noACK)
{
	(void)noACK;	// not implemented
	const char *datap = static_cast<char const *>(data);
	char *dataToSend = const_cast<char *>(datap);
	unsigned char retry = 0;
	uint16_t res = PJON_FAIL;
	while(retry++ < MY_PJON_MAX_RETRIES) {
		bus.receive();
		// This is how many times to try and transmit before failing.
		res = bus.send_packet_blocking(to, dataToSend, length);
		PJON_DEBUG(PSTR("PJON:SEND:TO=%,LEN=%,RETRY=%/%\n"), to, length, retry, MY_PJON_MAX_RETRIES);
		switch(res) {
				case PJON_ACK: PJON_DEBUG(PSTR("PJON:SEND:ACK\n")); break;
				case PJON_BUSY: PJON_DEBUG(PSTR("!PJON:SEND:BUSY\n")); break;
				case PJON_FAIL: PJON_DEBUG(PSTR("!PJON:SEND:FAIL\n")); break;
				default: PJON_DEBUG(PSTR("!PJON:SEND:RESPONSE=%\n"), res);
		}
		if (res == PJON_ACK) return true;
	}
	PJON_DEBUG(PSTR("!PJON:SEND:TO=%,NACK\n"), to);
	return res == PJON_ACK;
}

void _receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info) {
	PJON_DEBUG(PSTR("PJON:RECEIVE:TO=%,LEN=%\n"), packet_info.receiver_id, length);
	if (!_packet_received) {
		_packet_len = length;
		_packet_received = true;
		memcpy(_data, (const void *)payload, length);
	}
}

bool transportInit(void)
{
	PJON_DEBUG(PSTR("PJON:INIT\n"));
	bus.begin();
	bus.set_receiver(_receiver_function);
	bus.strategy.set_pin(MY_PJON_PIN);
	PJON_DEBUG(PSTR("PJON:INIT:PIN=%\n"), MY_PJON_PIN);
	return true;
}

void transportSetAddress(const uint8_t address)
{
	bus.set_id(address);
}

uint8_t transportGetAddress(void)
{
  return bus.device_id();
}


bool transportAvailable(void)
{
	bus.receive();
	return _packet_received;
}

bool transportSanityCheck(void)
{
	// not implemented yet
	return true;
}

uint8_t transportReceive(void* data)
{
	if (_packet_received) {
		memcpy(data,_data,_packet_len);
		_packet_received = false;
		return _packet_len;
	}
	else 
	{
		return (0);
	}
}

void transportPowerDown(void)
{
	// Nothing to shut down here
}

void transportPowerUp(void)
{
	// not implemented
}

void transportSleep(void)
{
	// not implemented
}

void transportStandBy(void)
{
	// not implemented
}

int16_t transportGetSendingRSSI(void)
{
	// not implemented
	return INVALID_RSSI;
}

int16_t transportGetReceivingRSSI(void)
{
	// not implemented
	return INVALID_RSSI;
}

int16_t transportGetSendingSNR(void)
{
	// not implemented
	return INVALID_SNR;
}

int16_t transportGetReceivingSNR(void)
{
	// not implemented
	return INVALID_SNR;
}

int16_t transportGetTxPowerPercent(void)
{
	// not implemented
	return static_cast<int16_t>(100);
}

int16_t transportGetTxPowerLevel(void)
{
	// not implemented
	return static_cast<int16_t>(100);
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	// not possible
	(void)powerPercent;
	return false;
}
