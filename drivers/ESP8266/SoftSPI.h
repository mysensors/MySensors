/* Arduino SoftSPI Library adapted for ESP8266
 * Copyright (C) 2013 by William Greiman
 * Copyright (C) 2017 by Vladimir Dronnikov
 *
 * This file is part of the Arduino DigitalIO Library
 *
 * This Library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Arduino DigitalIO Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file
 * @brief  Software SPI.
 *
 * @defgroup softSPI Software SPI
 * @ingroup internals
 * @details  Software SPI Template Class.
 * @{
 */

#ifndef SoftSPI_h
#define SoftSPI_h

//------------------------------------------------------------------------------
/**
 * @class SoftSPI
 * @brief Fast software SPI.
 */
template<uint8_t MisoPin, uint8_t MosiPin, uint8_t SckPin, uint8_t Mode = 0>
class SoftSPI
{
public:
	//----------------------------------------------------------------------------
	/** Initialize SoftSPI pins. */
	void begin()
	{
		pinMode(MisoPin, INPUT);
		pinMode(MosiPin, OUTPUT);
		pinMode(SckPin, OUTPUT);
	}
	//----------------------------------------------------------------------------
	/** Soft SPI transfer byte.
	 * @param[in] txData Data byte to send.
	 * @return Data byte received.
	 */
	uint8_t transfer(uint8_t txData)
	{
		uint8_t rxData, bits = 8;
		do {
			digitalWrite(MosiPin, txData & 0x80);
			digitalWrite(SckPin, HIGH);
			rxData <<= 1;
			if (digitalRead(MisoPin)) ++rxData;
			txData <<= 1;
			digitalWrite(SckPin, LOW);
		} while (--bits);
		return rxData;
	}
};
#endif  // SoftSPI_h
/** @} */
