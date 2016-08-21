/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyright (C) 2016 Marcelo Aquino
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef SerialPort_h
#define SerialPort_h

#include <string>
#include "Stream.h"

/**
 * SerialPort Class
 * Class that provides the functionality of arduino Serial library
 */
class SerialPort : public Stream {

private:
	int sd; //!< @brief file descriptor number.
	std::string serialPort;	//!< @brief tty name.

public:
	/**
	 * @brief SerialPort constructor.
	 */
	SerialPort();
	/**
	 * @brief SerialPort constructor.
	 */
	SerialPort(const char *port);
	/**
	* @brief Sets the data rate in bits per second (baud) for serial data transmission.
	*
	* @param serialSpeed serial port speed.
	*/
	void begin(int serialSpeed);
	/**
	* @brief Get the number of bytes available.
	*
	* Get the numberof bytes (characters) available for reading from
	* the serial port.
	*
	* @return number of bytes avalable to read.
	*/
	int available();
	/**
	* @brief Reads 1 byte of incoming serial data.
	*
	* @return first byte of incoming serial data available.
	*/
	int read();
	/**
	* @brief Writes a single byte to the serial port.
	*
	* @param b byte to write.
	* @return number of bytes written.
	*/
	size_t write(uint8_t b);
	/**
	* @brief Writes binary data to the serial port.
	*
	* @param buffer to write.
	* @param size of the buffer.
	* @return number of bytes written.
	*/
	size_t write(const uint8_t *buffer, size_t size);
	/**
	* @brief
	*
	* Returns the next byte (character) of incoming serial data without removing it from
	* the internal serial buffer.
	*
	* @return -1 if no data else character in the buffer.
	*/
	int peek();
	/**
	* @brief Remove any data remaining on the serial buffer.
	*/
	void flush();
	/**
	* @brief Disables serial communication.
	*/
	void end();
};

#endif
