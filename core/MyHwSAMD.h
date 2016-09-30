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
#ifndef MyHwSAMD_h
#define MyHwSAMD_h

#include "MyHw.h"
#include <Wire.h>

#ifdef __cplusplus
#include <Arduino.h>
//#include <SPI.h>
#endif

#include <avr/dtostrf.h>
#define I2C_EEP_ADDRESS 0x50

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
#define snprintf_P(s, f, ...) snprintf((s), (f), __VA_ARGS__)


// Define these as macros to save valuable space

uint8_t configBlock[1024];
#define hwDigitalWrite(__pin, __value) (digitalWrite(__pin, __value))
void hwInit();
void hwWatchdogReset();
void hwReboot();
#define hwMillis() millis()
#define hwRandomNumberInit() randomSeed(analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN))

void hwReadConfigBlock(void* buf, void* adr, size_t length);
void hwWriteConfigBlock(void* buf, void* adr, size_t length);
void hwWriteConfig(int adr, uint8_t value);
uint8_t hwReadConfig(int adr);

#define MY_SERIALDEVICE SerialUSB
#define MY_DEBUG_BUFFER_SIZE 300


/*
#define hwReadConfigBlock(__buf, __adr, __length) ( __length = __length)
#define hwWriteConfigBlock(__buf, __adr, __length) ( __length = __length)
#define hwWriteConfig(__adr, __value) ( __value = __value)
#define hwReadConfig(__adr) (0)
*/

/**
 * Disable all interrupts.
 * Helper function for MY_CRITICAL_SECTION.
 */
static __inline__ uint8_t __disableIntsRetVal(void)
{
    __disable_irq();
    return 1;
}
   
/** 
 * Restore priority mask register.
 * Helper function for MY_CRITICAL_SECTION.
 */
static __inline__ void __priMaskRestore(const uint32_t *priMask)
{
    __set_PRIMASK(*priMask);
}

#ifndef DOXYGEN
	#define MY_CRITICAL_SECTION    for ( uint32_t __savePriMask __attribute__((__cleanup__(__priMaskRestore))) = __get_PRIMASK(), __ToDo = __disableIntsRetVal(); __ToDo ; __ToDo = 0 )
#endif  /* DOXYGEN */

#endif // #ifdef ARDUINO_ARCH_SAMD
