/**
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
*/

#include "MyHwASR650x.h"

static volatile bool lowpower = false;
static volatile uint8_t wakeUpInterrupt1 = INVALID_INTERRUPT_NUM;
static volatile uint8_t wakeUpInterrupt2 = INVALID_INTERRUPT_NUM;
static volatile int8_t wakeUpReason = INVALID_INTERRUPT_NUM;
static TimerEvent_t wakeUp;

bool hwInit(void)
{
	//SpiInit();
	Asr_Timer_Init();
	RtcInit();
#if defined(CubeCell_Board)||defined(CubeCell_Capsule)||defined(CubeCell_BoardPlus)||defined(CubeCell_GPS)||defined(CubeCell_HalfAA)
	pinMode(Vext,OUTPUT);
	digitalWrite(Vext,HIGH);
	pinMode(VBAT_ADC_CTL,OUTPUT);
	digitalWrite(VBAT_ADC_CTL,HIGH);
#endif

#if !defined(MY_DISABLED_SERIAL)
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {
	}
#endif
#endif
	return true;
}

void hwWatchdogReset(void)
{
	// TODO: Not supported!
}

void hwReboot(void)
{
	HW_Reset(0);
	while (true)
		;
}

void onTimerWakeup()
{
	lowpower = false;
	wakeUpReason = MY_WAKE_UP_BY_TIMER;
}

void onInterrupt1Wakeup()
{
	lowpower = false;
	wakeUpReason = wakeUpInterrupt1;
	ClearPinInterrupt(wakeUpInterrupt1);
};

void onInterrupt2Wakeup()
{
	lowpower = false;
	wakeUpReason = wakeUpInterrupt2;
	ClearPinInterrupt(wakeUpInterrupt2);
};

int8_t hwSleep(uint32_t ms)
{
	wakeUpReason = INVALID_INTERRUPT_NUM;
	lowpower = true;
	wakeUp.Callback = onTimerWakeup;
	if (ms > 0) {
		TimerReset(&wakeUp);
		TimerSetValue(&wakeUp, ms);
		TimerStart(&wakeUp);
	}
	while (lowpower) {
		lowPowerHandler;
	}
	return wakeUpReason;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	wakeUpReason = INVALID_INTERRUPT_NUM;
	lowpower = true;
	wakeUpInterrupt1 = interrupt;
	attachInterrupt(wakeUpInterrupt1, onInterrupt1Wakeup, (IrqModes)mode);
	wakeUp.Callback = onTimerWakeup;
	if (ms > 0) {
		TimerReset(&wakeUp);
		TimerSetValue(&wakeUp, ms);
		TimerStart(&wakeUp);
	}
	while (lowpower) {
		lowPowerHandler;
	}
	//TimerStop(&wakeUp);
	//detachInterrupt(interrupt);
	return wakeUpReason;
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
               const uint8_t mode2,
               uint32_t ms)
{
	wakeUpReason = INVALID_INTERRUPT_NUM;
	lowpower = true;
	wakeUpInterrupt1 = interrupt1;
	wakeUpInterrupt2 = interrupt2;
	attachInterrupt(wakeUpInterrupt1, onInterrupt1Wakeup, (IrqModes)mode1);
	attachInterrupt(wakeUpInterrupt2, onInterrupt1Wakeup, (IrqModes)mode2);
	wakeUp.Callback = onTimerWakeup;
	if (ms > 0) {
		TimerReset(&wakeUp);
		TimerSetValue(&wakeUp, ms);
		TimerStart(&wakeUp);
	}
	while (lowpower) {
		lowPowerHandler;
	}
	TimerStop(&wakeUp);
	//detachInterrupt(interrupt1);
	//detachInterrupt(interrupt2);
	return wakeUpReason;
}

uint8_t hwReadConfig(const int pos)
{
	uint8_t data;
	FLASH_read_at(CY_SFLASH_USERBASE + pos, &data, 1);
	return data;
}

void hwWriteConfig(const int pos, uint8_t value)
{
	//don't write beyond 3*256 bytes Userflash
	if (pos >= 768) {
		return;
	}
	uint8_t data = hwReadConfig(pos);
	//only write if new content is different to save flash write cycles
	if (value != data) {
		FLASH_update(CY_SFLASH_USERBASE + pos, &value, 1);
	}
}
void hwReadConfigBlock(void *buffer, const void *pos, size_t length)
{
	FLASH_read_at(CY_SFLASH_USERBASE + (uint32_t)pos, static_cast<uint8_t *>(buffer),
	              length);
}

void hwWriteConfigBlock(void *buffer, const void *pos, size_t length)
{
	//don't write beyond 3*256 bytes Userflash
	if ((uint32_t)pos + length >= 768) {
		return;
	}
	uint8_t *buf = static_cast<uint8_t *>(buffer);
	uint8_t flash[length];
	hwReadConfigBlock(flash, pos, length);
	for (int i = 0; i < length; i++) {
		//only write if new content is different to save flash write cycles
		if (buf[i] != flash[i]) {
			FLASH_update(CY_SFLASH_USERBASE + (uint32_t)pos, static_cast<uint8_t *>(buffer),
			             length);
			return;
		}
	}
}

bool hwUniqueID(unique_id_t *uniqueID)
{
	BoardGetUniqueId(reinterpret_cast<uint8_t *>(uniqueID));
	(void)memset(reinterpret_cast<uint8_t *>(uniqueID) + 8, MY_HWID_PADDING_BYTE, 8);
	return true;
}

uint16_t hwCPUVoltage(void)
{
	return FUNCTION_NOT_SUPPORTED;
}

uint16_t hwCPUFrequency(void)
{
	return 42000000;
}

int8_t hwCPUTemperature(void)
{
	return -127; // not implemented yet
}

uint16_t hwFreeMem(void)
{
	// TODO: Not supported!
	return FUNCTION_NOT_SUPPORTED;
}

void hwRandomNumberInit(void)
{
	srand(HW_GetRandomSeed());
}
