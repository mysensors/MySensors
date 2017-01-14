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

#ifndef SerialSimulator_h
#define SerialSimulator_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Stream.h"

/**
 * @brief A class equivalent to Serial in Arduino but outputs to stdout
 */
class SerialSimulator : public Stream
{

public:
	/**
	 * @brief This function does nothing.
	 *
	 * @param baud Ignored parameter.
	 */
	void begin(int baud);
	/**
	 * @brief This function does nothing.
	 *
	 * @return always returns 1.
	 */
	int available();
	/**
	 * @brief Reads 1 key pressed from the keyboard.
	 *
	 * @return key character pressed cast to an int.
	 */
	int read();
	/**
	 * @brief Writes a single byte to stdout.
	 *
	 * @param b byte to write.
	 * @return -1 if error else, number of bytes written.
	 */
	size_t write(uint8_t b);
	/**
	 * @brief Not supported.
	 *
	 * @return always returns -1.
	 */
	int peek();
	/**
	 * @brief Flush stdout.
	 */
	void flush();
	/**
	 * @brief Nothing to do, flush stdout.
	 */
	void end();
};

#endif
