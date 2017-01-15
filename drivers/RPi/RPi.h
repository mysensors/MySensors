/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef RPi_h
#define	RPi_h

#include <stdint.h>
#include "bcm2835.h"
#include "cpuinfo.h"

#define INPUT BCM2835_GPIO_FSEL_INPT
#define OUTPUT BCM2835_GPIO_FSEL_OUTP

/**
 * @brief RPi class
 */
class RPi
{

public:
	/**
	 * @brief RPi constructor.
	 */
	RPi();
	/**
	 * @brief RPi destructor.
	 */
	~RPi();
	/**
	 * @brief Configures the specified pin to behave either as an input or an output.
	 *
	 * @param physPin The physical number of the pin.
	 * @param mode INPUT or OUTPUT.
	 */
	void pinMode(uint8_t physPin, uint8_t mode);
	/**
	 * @brief Write a high or a low value for the given pin.
	 *
	 * @param physPin The physical number of the pin.
	 * @param value HIGH or LOW.
	 */
	void digitalWrite(uint8_t physPin, uint8_t value);
	/**
	 * @brief Reads the value from a specified pin.
	 *
	 * @param physPin The physical number of the pin.
	 * @return HIGH or LOW.
	 */
	uint8_t digitalRead(uint8_t physPin);
	/**
	 * @brief Translate the physical pin number to the GPIO number for use in interrupt.
	 *
	 * @param physPin The physical number of the pin.
	 * @return The GPIO pin number.
	 */
	uint8_t digitalPinToInterrupt(uint8_t physPin);
	/**
	 * @brief Translate the physical pin number to the GPIO number.
	 *
	 * @param physPin The physical number of the pin.
	 * @param gpio Pointer to write the GPIO pin number when success.
	 * @return -1 if FAILURE or 0 if SUCCESS.
	 */
	static int physToGPIO(uint8_t physPin, uint8_t *gpio);

private:
	static const int *pin_to_gpio; //!< @brief Pointer to array of GPIO pins numbers.
	static rpi_info rpiinfo; //!< @brief Board information.
};

extern RPi rpi;

#endif
