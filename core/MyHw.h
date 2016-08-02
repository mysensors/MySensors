/*
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

 /**
 * @file MyHw.h
 *
 * MySensors hardware abstraction layer
 */
 
#ifndef MyHw_h
#define MyHw_h

#include "MyConfig.h"
#include "MyMessage.h"


#ifdef __cplusplus
#include <Arduino.h>
#endif

// Implement these as functions or macros
/*
#define hwDigitalWrite(__pin, __value)
#define hwInit() MY_SERIALDEVICE.begin(BAUD_RATE)
#define hwWatchdogReset() wdt_reset()
#define hwReboot() wdt_enable(WDTO_15MS); while (1)
#define hwMillis() millis()

void hwReadConfigBlock(void* buf, void* adr, size_t length);
void hwWriteConfigBlock(void* buf, void* adr, size_t length);
void hwWriteConfig(int adr, uint8_t value);
uint8_t hwReadConfig(int adr);
*/

/**
 * Sleep for a defined time, using minimum power.
 * @param ms   Time to sleep, in [ms].
 * @return Nonsense, please ignore.
 */
int8_t hwSleep(unsigned long ms);

/**
 * Sleep for a defined time, using minimum power, or until woken by interrupt.
 * @param interrupt  Interrupt number, which can wake the mcu from sleep.
 * @param mode       Interrupt mode, as passed to attachInterrupt.
 * @param ms         Time to sleep, in [ms].
 * @return -1 when woken by timer, or interrupt number when woken by interrupt.
 */
int8_t hwSleep(uint8_t interrupt, uint8_t mode, unsigned long ms);

/**
 * Sleep for a defined time, using minimum power, or until woken by one of the interrupts.
 * @param interrupt1  Interrupt1 number, which can wake the mcu from sleep.
 * @param mode1       Interrupt1 mode, as passed to attachInterrupt.
 * @param interrupt2  Interrupt2 number, which can wake the mcu from sleep.
 * @param mode2       Interrupt2 mode, as passed to attachInterrupt.
 * @param ms          Time to sleep, in [ms].
 * @return -1 when woken by timer, or interrupt number when woken by interrupt.
 */
int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms);

#ifdef MY_DEBUG
	void hwDebugPrint(const char *fmt, ... );
#endif

/** 
 * @def MY_CRITICAL_SECTION
 * @brief Creates a block of code that is guaranteed to be executed atomically.
 * Upon entering the block all interrupts are disabled, and re-enabled upon
 * exiting the block from any exit path.
 * A typical example that requires atomic access is a 16 (or more) bit variable
 * that is shared between the main execution path and an ISR, on an 8-bit
 * platform (e.g AVR):
 * @code
 * volatile uint16_t val = 0;
 * 
 * void interrupHandler()
 * {
 *   val = ~val;
 * }
 * 
 * void loop()
 * {
 *   uint16_t copy_val;
 *   MY_CRITICAL_SECTION
 *   {
 *     copy_val = val;
 *   }
 * }
 * @endcode
 * All code within the MY_CRITICAL_SECTION block will be protected from being
 * interrupted during execution.
 */
#ifdef DOXYGEN
	#define MY_CRITICAL_SECTION
#endif  /* DOXYGEN */

#endif // #ifdef MyHw_h
