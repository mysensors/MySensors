/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2026 Sensnology AB
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

#include "interrupt.h"
#include "GPIO.h"

void attachInterrupt(uint8_t gpioPin, void (*func)(), uint8_t mode)
{
	GPIO.attachInterrupt(gpioPin, func, mode);
}

void detachInterrupt(uint8_t gpioPin)
{
	GPIO.detachInterrupt(gpioPin);
}

void interrupts()
{
	GPIO.interrupts();
}

void noInterrupts()
{
	GPIO.noInterrupts();
}
