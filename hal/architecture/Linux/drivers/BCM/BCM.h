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

#ifndef BCM_h
#define	BCM_h

#include <stdint.h>
#include "bcm2835.h"

#define INPUT BCM2835_GPIO_FSEL_INPT
#define OUTPUT BCM2835_GPIO_FSEL_OUTP

/**
 * @brief BCM class
 */
class BCMClass
{

public:
	/**
	 * @brief BCMClass destructor.
	 */
	~BCMClass();
	/**
	 * @brief Initializes BCM.
	 *
	 * @return 1 if successful, else exits the program.
	 */
	uint8_t init();
	/**
	 * @brief Configures the specified pin to behave either as an input or an output.
	 *
	 * @param gpio The GPIO pin number.
	 * @param mode INPUT or OUTPUT.
	 */
	void pinMode(uint8_t gpio, uint8_t mode);
	/**
	 * @brief Write a high or a low value for the given pin.
	 *
	 * @param gpio The GPIO pin number.
	 * @param value HIGH or LOW.
	 */
	void digitalWrite(uint8_t gpio, uint8_t value);
	/**
	 * @brief Reads the value from a specified pin.
	 *
	 * @param gpio The GPIO pin number.
	 * @return HIGH or LOW.
	 */
	uint8_t digitalRead(uint8_t gpio);
	/**
	 * @brief Returns the same GPIO, no conversion is required.
	 *
	 * @param gpio The GPIO pin number.
	 * @return The GPIO pin number.
	 */
	inline uint8_t digitalPinToInterrupt(uint8_t gpio);
	/**
	 * @brief Checks if SPI was initialized.
	 *
	 * @return 1 if initialized, else 0.
	 */
	uint8_t isInitialized();

private:
	static uint8_t initialized; //!< @brief BCM initialized flag.
};

uint8_t BCMClass::digitalPinToInterrupt(uint8_t gpio)
{
	return gpio;
}

extern BCMClass BCM;

#endif
