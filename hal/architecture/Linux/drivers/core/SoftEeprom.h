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

/**
* This a software emulation of EEPROM that uses a file for data storage.
* A copy of the eeprom values are also held in memory for faster reading.
*/

#ifndef SoftEeprom_h
#define SoftEeprom_h

#include <stdint.h>

/**
 * SoftEeprom class
 */
class SoftEeprom
{

public:
	/**
	 * @brief SoftEeprom constructor.
	 */
	SoftEeprom();
	/**
	 * @brief SoftEeprom copy constructor.
	 */
	SoftEeprom(const SoftEeprom& other);
	/**
	 * @brief SoftEeprom destructor.
	 */
	~SoftEeprom();
	/**
	 * @brief Initializes the eeprom class.
	 *
	 * @param fileName filepath where the data is saved.
	 * @param length eeprom size in bytes.
	 * @return 0 if SUCCESS or -1 if FAILURE.
	 */
	int init(const char *fileName, size_t length);
	/**
	 * @brief Clear all allocated memory variables.
	 *
	 */
	void destroy();
	/**
	 * @brief Read a block of bytes from eeprom.
	 *
	 * @param buf buffer to copy to.
	 * @param addr eeprom address to read from.
	 * @param length number of bytes to read.
	 */
	void readBlock(void* buf, void* addr, size_t length);
	/**
	 * @brief Write a block of bytes to eeprom.
	 *
	 * @param buf buffer to read from.
	 * @param addr eeprom address to write to.
	 * @param length number of bytes to write.
	 */
	void writeBlock(void* buf, void* addr, size_t length);
	/**
	 * @brief Read a byte from eeprom.
	 *
	 * @param addr eeprom address to read from.
	 * @return the read byte.
	 */
	uint8_t readByte(int addr);
	/**
	 * @brief Write a byte to eeprom.
	 *
	 * @param addr eeprom address to write to.
	 * @param value to write.
	 */
	void writeByte(int addr, uint8_t value);
	/**
	 * @brief Overloaded assign operator.
	 *
	 */
	SoftEeprom& operator=(const SoftEeprom& other);

private:
	size_t _length; //!< @brief Eeprom max size.
	char *_fileName; //!< @brief file where the eeprom values are stored.
	uint8_t *_values; //!< @brief copy of the eeprom values held in memory for a faster reading.
};

#endif
