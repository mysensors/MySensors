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
#ifndef MyHwESP8266_h
#define MyHwESP8266_h

#include <SPI.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#ifdef __cplusplus
#include <Arduino.h>
#endif

#define CRYPTO_LITTLE_ENDIAN

#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

#ifndef MY_DEBUGDEVICE
#define MY_DEBUGDEVICE MY_SERIALDEVICE
#endif

#define EEPROM_size (1024)

// Define these as macros to save valuable space
#define hwDigitalWrite(__pin, __value) digitalWrite(__pin, __value)
#define hwDigitalRead(__pin) digitalRead(__pin)
#define hwPinMode(__pin, __value) pinMode(__pin, __value)
#define hwWatchdogReset() wdt_reset()
#define hwReboot() ESP.restart()
#define hwMillis() millis()
// The use of randomSeed switch to pseudo random number. Keep hwRandomNumberInit empty
#define hwRandomNumberInit()

bool hwInit(void);
void hwReadConfigBlock(void *buf, void *addr, size_t length);
void hwWriteConfigBlock(void *buf, void *addr, size_t length);
void hwWriteConfig(const int addr, uint8_t value);
uint8_t hwReadConfig(const int addr);
ssize_t hwGetentropy(void *__buffer, size_t __length);
//#define MY_HW_HAS_GETENTROPY

// SOFTSPI
#ifdef MY_SOFTSPI
#error Soft SPI is not available on this architecture!
#endif
#define hwSPI SPI //!< hwSPI


/**
 * Restore interrupt state.
 * Helper function for MY_CRITICAL_SECTION.
 */
static __inline__ void __psRestore(const uint32_t *__s)
{
	xt_wsr_ps( *__s );
}

#ifndef DOXYGEN
#define MY_CRITICAL_SECTION    for ( uint32_t __psSaved __attribute__((__cleanup__(__psRestore))) = xt_rsil(15), __ToDo = 1; __ToDo ; __ToDo = 0 )
#endif  /* DOXYGEN */

#endif // #ifdef ARDUINO_ARCH_ESP8266
