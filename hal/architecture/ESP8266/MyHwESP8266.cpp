/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwESP8266.h"
#include <EEPROM.h>

bool hwInit(void)
{
#if !defined(MY_DISABLED_SERIAL)
	MY_SERIALDEVICE.begin(MY_BAUD_RATE, SERIAL_8N1, MY_ESP8266_SERIAL_MODE, 1);
	MY_SERIALDEVICE.setDebugOutput(true);
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {}
#endif
#endif
	EEPROM.begin(EEPROM_size);
	return true;
}

void hwReadConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	int pos = reinterpret_cast<int>(addr);
	while (length-- > 0) {
		*dst++ = EEPROM.read(pos++);
	}
}

void hwWriteConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	int pos = reinterpret_cast<int>(addr);
	while (length-- > 0) {
		EEPROM.write(pos++, *src++);
	}
	// see implementation, commit only executed if diff
	EEPROM.commit();
}

uint8_t hwReadConfig(const int addr)
{
	uint8_t value;
	hwReadConfigBlock(&value, reinterpret_cast<void *>(addr), 1);
	return value;
}

void hwWriteConfig(const int addr, uint8_t value)
{
	hwWriteConfigBlock(&value, reinterpret_cast<void *>(addr), 1);
}

bool hwUniqueID(unique_id_t *uniqueID)
{
	// padding
	(void)memset((uint8_t *)uniqueID, MY_HWID_PADDING_BYTE, sizeof(unique_id_t));
	uint32_t val = ESP.getChipId();
	(void)memcpy((uint8_t *)uniqueID, &val, 4);
	val = ESP.getFlashChipId();
	(void)memcpy((uint8_t *)uniqueID + 4, &val, 4);
	return true;
}

ssize_t hwGetentropy(void *__buffer, size_t __length)
{
	// cut length if > 256
	if (__length > 256) {
		__length = 256;
	}
	uint8_t *dst = (uint8_t *)__buffer;

	// Start random number generator
	for (size_t i = 0; i < __length; i++) {
		dst[i] = (uint8_t)RANDOM_REG32;
	}

	return __length;
}

int8_t hwSleep(uint32_t ms)
{
	// TODO: Not supported!
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	// TODO: Not supported!
	(void)interrupt;
	(void)mode;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
               const uint8_t mode2,
               uint32_t ms)
{
	// TODO: Not supported!
	(void)interrupt1;
	(void)mode1;
	(void)interrupt2;
	(void)mode2;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

#if defined(MY_SPECIAL_DEBUG)
// settings for getVcc()
ADC_MODE(ADC_VCC);
#else
// [default] settings for analogRead(A0)
ADC_MODE(ADC_TOUT);
#endif

uint16_t hwCPUVoltage(void)
{
#if defined(MY_SPECIAL_DEBUG)
	// in mV, requires ADC_VCC set
	return ESP.getVcc();
#else
	// not possible
	return FUNCTION_NOT_SUPPORTED;
#endif
}

uint16_t hwCPUFrequency(void)
{
	// in 1/10Mhz
	return ESP.getCpuFreqMHz()*10;
}

int8_t hwCPUTemperature(void)
{
	return -127; // not available
}

uint16_t hwFreeMem(void)
{
	return ESP.getFreeHeap();
}
