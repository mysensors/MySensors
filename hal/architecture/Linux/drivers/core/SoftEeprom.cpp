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

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include "log.h"
#include "SoftEeprom.h"

SoftEeprom::SoftEeprom() : _length(0), _fileName(NULL), _values(NULL)
{
}

SoftEeprom::SoftEeprom(const SoftEeprom& other)
{
	_fileName = strdup(other._fileName);

	_length = other._length;
	_values = new uint8_t[_length];
	for (size_t i = 0; i < _length; ++i) {
		_values[i] = other._values[i];
	}
}

SoftEeprom::~SoftEeprom()
{
	destroy();
}

int SoftEeprom::init(const char *fileName, size_t length)
{
	struct stat fileInfo;

	destroy();

	_fileName = strdup(fileName);
	if (_fileName == NULL) {
		logError("Error: %s\n", strerror(errno));
		return -1;
	}

	_length = length;
	_values = new uint8_t[_length];

	if (stat(_fileName, &fileInfo) != 0) {
		//File does not exist.  Create it.
		logInfo("EEPROM file %s does not exist, creating new file.\n", _fileName);
		std::ofstream myFile(_fileName, std::ios::out | std::ios::binary);
		if (!myFile) {
			logError("Unable to create config file %s.\n", _fileName);
			return -1;
		}
		// Fill the eeprom with 1s
		for (size_t i = 0; i < _length; ++i) {
			_values[i] = 0xFF;
		}
		myFile.write((const char*)_values, _length);
		myFile.close();
	} else if (fileInfo.st_size < 0 || (size_t)fileInfo.st_size != _length) {
		logError("EEPROM file %s is not the correct size of %zu.  Please remove the file and a new one will be created.\n",
		         _fileName, _length);
		destroy();
		return -1;
	} else {
		//Read config into local memory.
		std::ifstream myFile(_fileName, std::ios::in | std::ios::binary);
		if (!myFile) {
			logError("Unable to open EEPROM file %s for reading.\n", _fileName);
			return -1;
		}
		myFile.read((char*)_values, _length);
		myFile.close();
	}

	return 0;
}

void SoftEeprom::destroy()
{
	if (_values) {
		delete[] _values;
	}
	if (_fileName) {
		free(_fileName);
	}
	_length = 0;
}

void SoftEeprom::readBlock(void* buf, void* addr, size_t length)
{
	unsigned long int offs = reinterpret_cast<unsigned long int>(addr);

	if (!length) {
		logError("EEPROM being read without being initialized!\n");
		return;
	}

	if (offs + length <= _length) {
		memcpy(buf, _values+offs, length);
	}
}

void SoftEeprom::writeBlock(void* buf, void* addr, size_t length)
{
	unsigned long int offs = reinterpret_cast<unsigned long int>(addr);

	if (!length) {
		logError("EEPROM being written without being initialized!\n");
		return;
	}

	if (offs + length <= _length) {
		if (memcmp(_values+offs, buf, length) == 0) {
			return;
		}

		memcpy(_values+offs, buf, length);

		std::ofstream myFile(_fileName, std::ios::out | std::ios::in | std::ios::binary);
		if (!myFile) {
			logError("Unable to write config to file %s.\n", _fileName);
			return;
		}
		myFile.seekp(offs);
		myFile.write((const char*)buf, length);
		myFile.close();
	}
}

uint8_t SoftEeprom::readByte(int addr)
{
	uint8_t value = 0xFF;
	readBlock(&value, reinterpret_cast<void*>(addr), 1);
	return value;
}

void SoftEeprom::writeByte(int addr, uint8_t value)
{
	uint8_t curr = readByte(addr);
	if (curr != value) {
		writeBlock(&value, reinterpret_cast<void*>(addr), 1);
	}
}

SoftEeprom& SoftEeprom::operator=(const SoftEeprom& other)
{
	if (this != &other) {
		delete[] _values;
		free(_fileName);

		_fileName = strdup(other._fileName);

		_length = other._length;
		_values = new uint8_t[_length];
		for (size_t i = 0; i < _length; ++i) {
			_values[i] = other._values[i];
		}
	}
	return *this;
}
