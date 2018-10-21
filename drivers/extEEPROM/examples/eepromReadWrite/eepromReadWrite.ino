/*
  Copyright (c) 2014 Arduino.  All right reserved.

  This library is free software; you can redistribute it and / or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110 - 1301  USA
*/

#include "extEEPROM.h"

extEEPROM myEEPROM(kbits_256, 1, 64, 0x57);

void setup(void)
{
	SerialUSB.begin(115200);
	while (!SerialUSB) {
		;
	}

	byte i2cStat = myEEPROM.begin(myEEPROM.twiClock100kHz);
	if ( i2cStat != 0 ) {
		SerialUSB.println(F("I2C Problem"));
	}

	SerialUSB.println(
	    F("EEPROM Memory commands:  read:(a)(l)(r) , write:(a)(d)(w), next read data (n)"));
	SerialUSB.println(F("- Commands TO PRESS:"));
	SerialUSB.println(F("\t a : memory address to read / write"));
	SerialUSB.println(F("\t d : data to write"));
	SerialUSB.println(F("\t l : data to write"));
	SerialUSB.println(F("\t r : read command"));
	SerialUSB.println(F("\t w : write command"));
}

unsigned long address = 0;
const unsigned int maxDataSize = 1024;  //0x8000;   // 32 k bytes (32768 = 0x8000) = 256 kbits

byte data[maxDataSize] = {'p', 'i', 'p', 'p', 'o'};
unsigned int dataSize = 5;

void eprom_read_write(bool write)
{
	byte i2cStat = 0;
	if (write) {
		i2cStat = myEEPROM.write(address, data, dataSize);
	} else {
		memset(data, 0, maxDataSize);
		i2cStat = myEEPROM.read(address, data, dataSize);
	}
	if ( i2cStat != 0 ) {
		//there was a problem
		SerialUSB.print(F("I2C Problem: "));
		if ( i2cStat == EEPROM_ADDR_ERR) {
			SerialUSB.println(F("Wrong address"));
		} else {
			SerialUSB.print(F("I2C error: "));
			SerialUSB.print(i2cStat);
			SerialUSB.println(F(""));
		}
	}
}


void parse(char inChar)
{
	const char addr_len = 5;
	char       addr_char[addr_len] = "";
	const char data_len = 3;
	char       data_char[data_len] = "";
	char       size_char[data_len] = "";
	char       inc = 0, i = 0, j = 0;

	switch (inChar) {
	case 'a':
		SerialUSB.print(F("Insert Address as 4 Hex chars (without '0x'):  "));

		while (i < 4) {
			while (SerialUSB.available() <= 0)
				;
			inc = SerialUSB.read();

			if (inc == 'q') {
				return;
			}

			addr_char[i] = inc;
			++i;
		}
		address = (unsigned long)strtol(addr_char, NULL, 16);
		SerialUSB.println(address);
		break;

	case 'd':
		SerialUSB.print(F("Insert Hex data sequence (without '0x'), return to enter: "));
		memset(data, 0, maxDataSize);
		while (true) {
			while (SerialUSB.available() <= 0)
				;
			inc = SerialUSB.read();
			if (inc == 'q') {
				return;
			}
			if (inc == '\r' || inc == '\n') {
				break;
			}

			if (inc >= 'a' && inc <= 'f') {
				data[j] += inc - 'a' + 10;
			} else  if (inc >= 'A' && inc <= 'F') {
				data[j] += inc - 'A' + 10;
			} else if (inc >= '0' && inc <= '9') {
				data[j] += inc - '0';
			} else {
				return;
			}

			if (i % 2) {
				j++;
			} else {
				data[j] = data[j] << 4;
			}
			i++;
		}
		dataSize = j;
		SerialUSB.println(dataSize);
		SerialUSB.println(F(""));
		break;
	case 'l':
		SerialUSB.print(F("Insert data len as 2 Hex chars (without '0x'): "));
		while (i < 2) {
			while (SerialUSB.available() <= 0)
				;
			inc = SerialUSB.read();
			if (inc == 'q') {
				return;
			}

			size_char[i] = inc;
			++i;
			if (inc == '\n') {
				return;
			}
		}

		dataSize = (unsigned int)strtol(size_char, NULL, 16);
		SerialUSB.println(dataSize);
		break;


	case 'n':
		address += dataSize;
	/* FALLTHROUGH */
	case 'r':
		SerialUSB.print(F("reading address: "));
		SerialUSB.println(address, HEX);

		eprom_read_write(false);
		for (i = 0; i < dataSize ; ++i) {
			SerialUSB.print(data[i], HEX);
			SerialUSB.print(F(" "));
		}
		SerialUSB.println();

		break;

	case 'w':
		SerialUSB.print(F("writing at address: "));
		SerialUSB.print(address, HEX);
		SerialUSB.print(F(", len: "));
		SerialUSB.println(address, dataSize);
		for (i = 0; i < dataSize ; ++i) {
			SerialUSB.print(data[i], HEX);
			SerialUSB.print(F(" "));
		}
		eprom_read_write(true);
		SerialUSB.println();

		break;
	case 'T':
		SerialUSB.println(F("Memory test: writing and verifying the whole memory"));
		break;

	default:
		break;
	}
}


void loop(void)
{
	if (SerialUSB.available() > 0) {
		char inChar = SerialUSB.read();
		SerialUSB.print(inChar);
		parse(inChar);
	}

	delay(10);
}

