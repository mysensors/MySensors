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

#include "MyHwESP8266.h"
#include <EEPROM.h>

/*
int8_t pinIntTrigger = 0;
void wakeUp()	 //place to send the interrupts
{
	pinIntTrigger = 1;
}
void wakeUp2()	 //place to send the second interrupts
{
	pinIntTrigger = 2;
}

// Watchdog Timer interrupt service routine. This routine is required
// to allow automatic WDIF and WDIE bit clearance in hardware.
ISR (WDT_vect)
{
	// WDIE & WDIF is cleared in hardware upon entering this ISR
	wdt_disable();
}
*/

static void hwInitConfigBlock( size_t length = 1024 /*ATMega328 has 1024 bytes*/ )
{
  static bool initDone = false;
  if (!initDone)
  {
    EEPROM.begin(length);
    initDone = true;
  }
}

void hwReadConfigBlock(void* buf, void* adr, size_t length)
{
  hwInitConfigBlock();
  uint8_t* dst = static_cast<uint8_t*>(buf);
  int offs = reinterpret_cast<int>(adr);
  while (length-- > 0)
  {
    *dst++ = EEPROM.read(offs++); 
  }
}

void hwWriteConfigBlock(void* buf, void* adr, size_t length)
{
  hwInitConfigBlock();
  uint8_t* src = static_cast<uint8_t*>(buf);
  int offs = reinterpret_cast<int>(adr);
  while (length-- > 0)
  {
    EEPROM.write(offs++, *src++);
  }
  EEPROM.commit();
}

uint8_t hwReadConfig(int adr)
{
  uint8_t value;
  hwReadConfigBlock(&value, reinterpret_cast<void*>(adr), 1);
  return value;
}

void hwWriteConfig(int adr, uint8_t value)
{
  uint8_t curr = hwReadConfig(adr);
  if (curr != value)
  {
    hwWriteConfigBlock(&value, reinterpret_cast<void*>(adr), 1);
  }
}


int8_t hwSleep(unsigned long ms) {
	// TODO: Not supported!
	(void)ms;
	return -2;
}

int8_t hwSleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	// TODO: Not supported!
	(void)interrupt;
	(void)mode;
	(void)ms;
	return -2;
}

int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	// TODO: Not supported!
	(void)interrupt1;
	(void)mode1;
	(void)interrupt2;
	(void)mode2;
	(void)ms;
	return -2;
}

ADC_MODE(ADC_VCC);

uint16_t hwCPUVoltage() {
	// in mV
	return ESP.getVcc();
}

uint16_t hwCPUFrequency() {
	// in 1/10Mhz
	return ESP.getCpuFreqMHz()*10;
}

#ifdef MY_DEBUG
void hwDebugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	#ifdef MY_GATEWAY_FEATURE
		// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
		snprintf_P(fmtBuffer, 299, PSTR("0;255;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
		Serial.print(fmtBuffer);
	#endif
	va_list args;
	va_start (args, fmt );
	va_end (args);
	#ifdef MY_GATEWAY_FEATURE
		// Truncate message if this is gateway node
		vsnprintf_P(fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH, fmt, args);
		fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH-1] = '\n';
		fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH] = '\0';
	#else
		vsnprintf_P(fmtBuffer, 299, fmt, args);
	#endif
	va_end (args);
	Serial.print(fmtBuffer);
	Serial.flush();

	//Serial.write(freeRam());
}
#endif
