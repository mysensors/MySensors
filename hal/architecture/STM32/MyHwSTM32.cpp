/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2025 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * @file MyHwSTM32.cpp
 * @brief Hardware abstraction layer for STM32 microcontrollers using STM32duino core
 *
 * This implementation uses the official STM32duino Arduino core which provides
 * STM32Cube HAL underneath. It supports a wide range of STM32 families including
 * F0, F1, F4, L0, L4, G0, G4, H7, and more.
 *
 * Tested on:
 * - STM32F401CC/CE Black Pill
 * - STM32F411CE Black Pill
 *
 * Pin Mapping Example (STM32F4 Black Pill):
 *
 * nRF24L01+ Radio (SPI1):
 * - SCK:  PA5
 * - MISO: PA6
 * - MOSI: PA7
 * - CSN:  PA4
 * - CE:   PB0 (configurable via MY_RF24_CE_PIN)
 *
 * RFM69/RFM95 Radio (SPI1):
 * - SCK:  PA5
 * - MISO: PA6
 * - MOSI: PA7
 * - CS:   PA4
 * - IRQ:  PA3 (configurable)
 * - RST:  PA2 (configurable)
 */

#include "MyHwSTM32.h"

bool hwInit(void)
{
#if !defined(MY_DISABLED_SERIAL)
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
	// Wait for serial port to connect (needed for native USB)
	while (!MY_SERIALDEVICE) {
		; // Wait for serial port connection
	}
#endif
#endif

	// STM32duino EEPROM library auto-initializes on first use
	// No explicit initialization required
	return true;
}

void hwReadConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	int pos = reinterpret_cast<int>(addr);

	for (size_t i = 0; i < length; i++) {
		dst[i] = EEPROM.read(pos + i);
	}
}

void hwWriteConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	int pos = reinterpret_cast<int>(addr);

	for (size_t i = 0; i < length; i++) {
		EEPROM.update(pos + i, src[i]);
	}

	// Commit changes to flash (STM32duino EEPROM emulation)
	// Note: This happens automatically on next read or explicit commit
}

uint8_t hwReadConfig(const int addr)
{
	return EEPROM.read(addr);
}

void hwWriteConfig(const int addr, uint8_t value)
{
	EEPROM.update(addr, value);
}

void hwWatchdogReset(void)
{
#if defined(HAL_IWDG_MODULE_ENABLED) && defined(IWDG)
	// Reset independent watchdog if enabled
	// Use direct register write to reload watchdog counter
	// This works whether IWDG was initialized by HAL or LL drivers
	IWDG->KR = IWDG_KEY_RELOAD;
#endif
	// No-op if watchdog not enabled
}

void hwReboot(void)
{
	NVIC_SystemReset();
}

void hwRandomNumberInit(void)
{
	// Use internal temperature sensor and ADC noise as entropy source
	// This provides reasonably good random seed values

#ifdef ADC1
	uint32_t seed = 0;

	// Read multiple samples from different sources for entropy
	for (uint8_t i = 0; i < 32; i++) {
		uint32_t value = 0;

#ifdef TEMP_SENSOR_AVAILABLE
		// Try to read internal temperature sensor if available
		value ^= analogRead(ATEMP);
#endif

#ifdef VREF_AVAILABLE
		// Mix in internal voltage reference reading
		value ^= analogRead(AVREF);
#endif

		// Mix in current time
		value ^= hwMillis();

		// Mix in system tick
		value ^= micros();

		// Accumulate into seed
		seed ^= (value & 0x7) << (i % 29);

		// Small delay to ensure values change
		delayMicroseconds(100);
	}

	randomSeed(seed);
#else
	// Fallback: use millis as weak entropy source
	randomSeed(hwMillis());
#endif // ADC1
}

bool hwUniqueID(unique_id_t *uniqueID)
{
#ifdef UID_BASE
	// STM32 unique device ID is stored at a fixed address
	// Length is 96 bits (12 bytes) but we store 16 bytes for compatibility

	uint32_t *id = (uint32_t *)UID_BASE;
	uint8_t *dst = (uint8_t *)uniqueID;

	// Copy 12 bytes of unique ID
	for (uint8_t i = 0; i < 12; i++) {
		dst[i] = ((uint8_t *)id)[i];
	}

	// Pad remaining bytes with zeros
	for (uint8_t i = 12; i < 16; i++) {
		dst[i] = 0;
	}

	return true;
#else
	// Unique ID not available on this variant
	return false;
#endif
}

uint16_t hwCPUVoltage(void)
{
#if defined(VREF_AVAILABLE) && defined(AVREF) && defined(__HAL_RCC_ADC1_CLK_ENABLE)
	// Read internal voltage reference to calculate VDD
	// VREFINT is typically 1.2V (varies by STM32 family)

	uint32_t vrefint = analogRead(AVREF);

	if (vrefint > 0) {
		// Calculate VDD in millivolts
		// Formula: VDD = 3.3V * 4096 / ADC_reading
		// Adjusted: VDD = 1200mV * 4096 / vrefint_reading
		return (uint16_t)((1200UL * 4096UL) / vrefint);
	}
#endif

	// Return typical 3.3V if measurement not available
	return 3300;
}

uint16_t hwCPUFrequency(void)
{
	// Return CPU frequency in 0.1 MHz units
	// F_CPU is defined by the build system (e.g., 84000000 for 84 MHz)
	return F_CPU / 100000UL;
}

int8_t hwCPUTemperature(void)
{
#if defined(TEMP_SENSOR_AVAILABLE) && defined(ATEMP) && defined(__HAL_RCC_ADC1_CLK_ENABLE)
	// Read internal temperature sensor
	// Note: Requires calibration values for accurate results

	int32_t temp_raw = analogRead(ATEMP);

#ifdef TEMP110_CAL_ADDR
	// Use factory calibration if available (STM32F4, L4, etc.)
	uint16_t *temp30_cal = (uint16_t *)TEMP30_CAL_ADDR;
	uint16_t *temp110_cal = (uint16_t *)TEMP110_CAL_ADDR;

	if (temp30_cal && temp110_cal && *temp110_cal != *temp30_cal) {
		// Calculate temperature using two-point calibration
		// Formula: T = ((110-30) / (CAL_110 - CAL_30)) * (raw - CAL_30) + 30
		int32_t temp = 30 + ((110 - 30) * (temp_raw - *temp30_cal)) /
		               (*temp110_cal - *temp30_cal);

		// Apply user calibration
		temp = (temp - MY_STM32_TEMPERATURE_OFFSET) / MY_STM32_TEMPERATURE_GAIN;

		return (int8_t)temp;
	}
#endif // TEMP110_CAL_ADDR

	// Fallback: use typical values (less accurate)
	// Typical slope: 2.5 mV/°C, V25 = 0.76V for STM32F4
	// This is a rough approximation
	float voltage = (temp_raw * 3.3f) / 4096.0f;
	int32_t temp = 25 + (int32_t)((voltage - 0.76f) / 0.0025f);

	return (int8_t)((temp - MY_STM32_TEMPERATURE_OFFSET) / MY_STM32_TEMPERATURE_GAIN);
#else
	// Temperature sensor not available
	return FUNCTION_NOT_SUPPORTED;
#endif
}

uint16_t hwFreeMem(void)
{
	// Calculate free heap memory
	// This uses newlib's mallinfo if available

#ifdef STACK_TOP
	extern char *__brkval;
	extern char __heap_start;

	char *heap_end = __brkval ? __brkval : &__heap_start;
	char stack_var;

	// Calculate space between heap and stack
	return (uint16_t)(&stack_var - heap_end);
#else
	// Alternative method: try to allocate and measure
	// Not implemented to avoid fragmentation
	return FUNCTION_NOT_SUPPORTED;
#endif
}

int8_t hwSleep(uint32_t ms)
{
	// TODO: Implement low-power sleep mode
	// For now, use simple delay
	// Future: Use STM32 STOP or STANDBY mode with RTC wakeup

	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	// TODO: Implement interrupt-based sleep
	// Future: Configure EXTI and enter STOP mode

	(void)interrupt;
	(void)mode;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1,
               const uint8_t interrupt2, const uint8_t mode2, uint32_t ms)
{
	// TODO: Implement dual-interrupt sleep

	(void)interrupt1;
	(void)mode1;
	(void)interrupt2;
	(void)mode2;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}
