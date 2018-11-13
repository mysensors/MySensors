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


bool transportSend(const uint8_t to, const void* data, const uint8_t len, const bool noACK)
{
	(void)noACK;	// not implemented
	const char *datap = static_cast<char const *>(data);
	char *dataToSend = const_cast<char *>(datap);
	unsigned char timeout = 5;
	uint16_t res = PJON_FAIL;
	while(timeout-->0) {
		bus.receive();
		// This is how many times to try and transmit before failing.
		res = bus.send_packet_blocking(to, dataToSend, len);
		#ifdef PJON_DEBUG
			Serial.print("PJON_SEND: to=");
			Serial.print(to, HEX);
			Serial.print(" len=");
			Serial.print(len, DEC);
			Serial.print(" -> ");
			for (char i = 0; i < len; i++) {
				if (datap[i] < 15) {
					Serial.print('0');
				}
				Serial.print(datap[i], HEX);
			}
			Serial.print(" res: ");
			switch(res) {
				case PJON_ACK: Serial.println("ACK"); break;
				case PJON_BUSY: Serial.println("BUSY"); break;
				case PJON_FAIL: Serial.println("FAIL"); break;
				default: Serial.println(res);
			}
		#endif
		if (res == PJON_ACK) break;
	}
	return res == PJON_ACK;
}

void _receiver_function(uint8_t *payload, uint16_t l, const PJON_Packet_Info &packet_info) {
  #ifdef PJON_DEBUG
	Serial.print("PJON_READ: ");
	Serial.print(packet_info.receiver_id);
	Serial.print(": ");
	for (char i = 0; i < l; i++) {
		if (payload[i] < 15) {
		Serial.print('0');
		}
		Serial.print(payload[i], HEX);
	}
	Serial.println();
  #endif
  if (!_packet_received) {
    _packet_len = l;
    _packet_received = true;
    memcpy(_data, (const void *)payload, l);
  }
};

bool transportInit(void)
{
	bus.begin();
	bus.set_receiver(_receiver_function);
	bus.strategy.set_pin(MY_PJON_PIN);
	return true;
}

void transportSetAddress(const uint8_t address)
{
	#ifdef PJON_DEBUG
	  Serial.print("PJON_ADDR: ");
	  Serial.println(address);
    #endif
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
	else {
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
