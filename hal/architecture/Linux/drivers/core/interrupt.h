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
 *
 * Based on wiringPi Copyright (c) 2012 Gordon Henderson.
 */

#ifndef interrupt_h
#define interrupt_h

#include <stdint.h>

#define CHANGE 1
#define FALLING 2
#define RISING 3
#define NONE 4

#ifdef __cplusplus
extern "C" {
#endif

void attachInterrupt(uint8_t gpioPin, void(*func)(), uint8_t mode);
void detachInterrupt(uint8_t gpioPin);
void interrupts();
void noInterrupts();

#ifdef __cplusplus
}
#endif

#endif
