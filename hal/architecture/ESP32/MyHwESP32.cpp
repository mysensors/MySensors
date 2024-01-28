/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2022 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Arduino core for ESP32: https://github.com/espressif/arduino-esp32
 *
 * MySensors ESP32 implementation, Copyright (C) 2017-2019 Olivier Mauti <olivier@mysensors.org>
 *
 */

#include "MyHwESP32.h"

bool hwInit(void)
{
#if !defined(MY_DISABLED_SERIAL)
	MY_SERIALDEVICE.begin(MY_BAUD_RATE, SERIAL_8N1);
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {}
#endif
#endif
	return EEPROM.begin(MY_EEPROM_SIZE);
}

void hwReadConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	int offs = reinterpret_cast<int>(addr);
	while (length-- > 0) {
		*dst++ = EEPROM.read(offs++);
	}
}

void hwWriteConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	int offs = reinterpret_cast<int>(addr);
	while (length-- > 0) {
		EEPROM.write(offs++, *src++);
	}
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
	if (hwReadConfig(addr) != value) {
		hwWriteConfigBlock(&value, reinterpret_cast<void *>(addr), 1);
	}
}

bool hwUniqueID(unique_id_t *uniqueID)
{
	uint64_t val = ESP.getEfuseMac();
	(void)memcpy(static_cast<void *>(uniqueID), (void *)&val, 8);
	(void)memset(static_cast<void *>(uniqueID + 8), MY_HWID_PADDING_BYTE, 8); // padding
	return true;
}

ssize_t hwGetentropy(void *__buffer, size_t __length)
{
	// cut length if > 256
	if (__length > 256) {
		__length = 256;
	}
	uint8_t *dst = static_cast<uint8_t *>(__buffer);
	// get random numbers
	for (size_t i = 0; i < __length; i++) {
		dst[i] = (uint8_t)esp_random();
	}
	return __length;
}

int8_t hwSleep(uint32_t ms)
{
	esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
	esp_light_sleep_start();
	return MY_WAKE_UP_BY_TIMER;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	if(mode ==  FALLING) {
		gpio_wakeup_enable((gpio_num_t)interrupt, GPIO_INTR_LOW_LEVEL);
	} else if (mode == RISING) {
		gpio_wakeup_enable((gpio_num_t)interrupt, GPIO_INTR_HIGH_LEVEL);
	} else {
		return MY_SLEEP_NOT_POSSIBLE;
	}
	esp_sleep_enable_gpio_wakeup();
	esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
	esp_light_sleep_start();
	gpio_wakeup_disable((gpio_num_t)interrupt);
	return 0;
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
               const uint8_t mode2,
               uint32_t ms)
{
	if(mode1 ==  FALLING) {
		gpio_wakeup_enable((gpio_num_t)interrupt1, GPIO_INTR_LOW_LEVEL);
	} else if (mode1 == RISING) {
		gpio_wakeup_enable((gpio_num_t)interrupt1, GPIO_INTR_HIGH_LEVEL);
	} else {
		return MY_SLEEP_NOT_POSSIBLE;
	}
	if(mode2 ==  FALLING) {
		gpio_wakeup_enable((gpio_num_t)interrupt2, GPIO_INTR_LOW_LEVEL);
	} else if (mode2 == RISING) {
		gpio_wakeup_enable((gpio_num_t)interrupt2, GPIO_INTR_HIGH_LEVEL);
	} else {
		return MY_SLEEP_NOT_POSSIBLE;
	}
	esp_sleep_enable_gpio_wakeup();
	esp_sleep_enable_timer_wakeup((uint64_t)ms * 1000);
	esp_light_sleep_start();
	gpio_wakeup_disable((gpio_num_t)interrupt1);
	gpio_wakeup_disable((gpio_num_t)interrupt2);
	return 0;
}

uint16_t hwCPUVoltage(void)
{
	// experimental, not documented feature and inaccurate?
	uint16_t internalBatReading;
	if (WiFi.status() == 255) {
		btStart();
		internalBatReading = rom_phy_get_vdd33();
		btStop();
	} else {
		internalBatReading = rom_phy_get_vdd33();
	}
	return internalBatReading;
}

uint16_t hwCPUFrequency(void)
{
	// in 1/10Mhz
	return static_cast<uint16_t>(ESP.getCpuFreqMHz() * 10);
}

int8_t hwCPUTemperature(void)
{
	// CPU temperature in °C
	return static_cast<int8_t>((temperatureRead() - MY_ESP32_TEMPERATURE_OFFSET) /
	                           MY_ESP32_TEMPERATURE_GAIN);
}

uint16_t hwFreeMem(void)
{
	return static_cast<uint16_t>(ESP.getFreeHeap());
}
