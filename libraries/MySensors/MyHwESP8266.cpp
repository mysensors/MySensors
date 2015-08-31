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

#ifdef ARDUINO_ARCH_ESP8266
 
#include "MyHw.h"
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

static void hw_initConfigBlock( size_t length = 1024 /*ATMega328 has 1024 bytes*/ )
{
  static bool initDone = false;
  if (!initDone)
  {
    EEPROM.begin(length);
    initDone = true;
  }
}

void hw_readConfigBlock(void* buf, void* adr, size_t length)
{
  hw_initConfigBlock();
  uint8_t* dst = static_cast<uint8_t*>(buf);
  int offs = reinterpret_cast<int>(adr);
  while (length-- > 0)
  {
    *dst++ = EEPROM.read(offs++); 
  }
}

void hw_writeConfigBlock(void* buf, void* adr, size_t length)
{
  hw_initConfigBlock();
  uint8_t* src = static_cast<uint8_t*>(buf);
  int offs = reinterpret_cast<int>(adr);
  while (length-- > 0)
  {
    EEPROM.write(offs++, *src++);
  }
  EEPROM.commit();
}

uint8_t hw_readConfig(int adr)
{
  uint8_t value;
  hw_readConfigBlock(&value, reinterpret_cast<void*>(adr), 1);
  return value;
}

void hw_writeConfig(int adr, uint8_t value)
{
  uint8_t curr = hw_readConfig(adr);
  if (curr != value)
  {
    hw_writeConfigBlock(&value, reinterpret_cast<void*>(adr), 1); 
  }
}



MyHwESP8266::MyHwESP8266() : MyHw()
{
}


/*

// The following was redifined as macros to save space

inline uint8_t MyHwATMega328::readConfig(uint8_t pos) {
	return eeprom_read_byte((uint8_t*)pos);
}

inline void MyHwATMega328::writeConfig(uint8_t pos, uint8_t value) {
	eeprom_update_byte((uint8_t*)pos, value);
}

inline void MyHwATMega328::readConfigBlock(void* buf, void * pos, size_t length) {
	eeprom_read_block(buf, (void*)pos, length);
}

inline void MyHwATMega328::writeConfigBlock(void* pos, void* buf, size_t length) {
	eeprom_write_block((void*)pos, (void*)buf, length);
}


inline void MyHwATMega328::init() {
	Serial.begin(BAUD_RATE);
}

inline void MyHwATMega328::watchdogReset() {
	wdt_reset();
}

inline void MyHwATMega328::reboot() {
	wdt_enable(WDTO_15MS); for (;;);
}

inline unsigned long MyHwATMega328::millis() {
	return ::millis();
}
*/


void MyHwESP8266::sleep(unsigned long ms) {
  // TODO: Not supported!
}

bool MyHwESP8266::sleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
  // TODO: Not supported!
	return false;
}

inline uint8_t MyHwESP8266::sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
  // TODO: Not supported!
	return 0;
}



#ifdef DEBUG
void MyHwESP8266::debugPrint(bool isGW, const char *fmt, ... ) {
	char fmtBuffer[300];
	if (isGW) {
		// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
		snprintf_P(fmtBuffer, 299, PSTR("0;0;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
		Serial.print(fmtBuffer);
	}
	va_list args;
	va_start (args, fmt );
	va_end (args);
	if (isGW) {
		// Truncate message if this is gateway node
		vsnprintf_P(fmtBuffer, 60, fmt, args);
		fmtBuffer[59] = '\n';
		fmtBuffer[60] = '\0';
	} else {
		vsnprintf_P(fmtBuffer, 299, fmt, args);
	}
	va_end (args);
	Serial.print(fmtBuffer);
	Serial.flush();

	//Serial.write(freeRam());
}
#endif

#endif  // #ifdef ARDUINO_ARCH_ESP8266
