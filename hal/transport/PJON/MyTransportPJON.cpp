/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2020 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * MySensors PJON transport layer integration: Adrian Sławiński
 * -------------------------------------------------------------------------------
 *
 */

// Set a higher polling duration if the device is executing long tasks
#define PJON_POLLING_DURATION 1000

PJONSoftwareBitBang bus;

// debug
#if defined(MY_DEBUG_VERBOSE_PJON)
#define PJON_DEBUG(x,...)	DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< Debug print
#else
#define PJON_DEBUG(x,...)	//!< DEBUG null
#endif

char _data[MAX_MESSAGE_SIZE];
uint8_t _packet_len;
unsigned char _packet_from;
bool _packet_received;

bool transportSend(const uint8_t to, const void *data, const uint8_t length, const bool noACK)
{
	const char *datap = static_cast<const char *>(data);
	char *dataToSend = const_cast<char *>(datap);
	uint16_t res = bus.send(to, dataToSend, length,
	                        (noACK) ? (bus.config & ~PJON_ACK_REQ_BIT) : PJON_NO_HEADER);
	bus.update();
	if(res == PJON_FAIL) {
		PJON_DEBUG(PSTR("!PJON:SND:FAIL\n"));
		return false;
	} else {
		return true;
	}
}

void _receiver_function(uint8_t *payload, uint16_t length, const PJON_Packet_Info &packet_info)
{
	PJON_DEBUG(PSTR("PJON:RCV:TO=%" PRIu8 ",LEN=%" PRIu8 "\n"), packet_info.rx.id, length);
	if (!_packet_received) {
		_packet_len = length;
		_packet_received = true;
		(void)memcpy((void *)_data, (const void *)payload, length);
	}
}

bool transportInit(void)
{
	PJON_DEBUG(PSTR("PJON:INIT:PIN=%" PRIu8 "\n"), MY_PJON_PIN);
	bus.begin();
	bus.set_receiver(_receiver_function);
	bus.strategy.set_pin(MY_PJON_PIN);
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


bool transportDataAvailable(void)
{
	bus.receive(PJON_POLLING_DURATION);
	bus.update();
	return _packet_received;
}

bool transportSanityCheck(void)
{
	// not implemented on PHY layer
	return true;
}

uint8_t transportReceive(void *data)
{
	if (_packet_received) {
		(void)memcpy(data, (const void *)_data, _packet_len);
		_packet_received = false;
	} else {
		_packet_len = 0;
	}
	return _packet_len;

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
