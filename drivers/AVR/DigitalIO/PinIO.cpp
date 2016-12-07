/* Arduino DigitalIO Library
 * Copyright (C) 2013 by William Greiman
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
 * @brief Digital AVR port I/O with runtime pin number.
 *
 * @defgroup runtimeDigital Runtime Pin I/O
 * @details  Two Wire Interface library.
 * @{
 */
#include "PinIO.h"
#include <util/atomic.h>
#include <Arduino.h>
//==============================================================================
/** Constructor
 * @param[in] pin Pin assigned to this object.
 */
PinIO::PinIO(uint8_t pin)
{
	begin(pin);
}
//------------------------------------------------------------------------------
/** Initialize pin bit mask and port address.
 * @param[in] pin Arduino board pin number.
 * @return true for success or false if invalid pin number.
 */
bool PinIO::begin(uint8_t pin)
{
	if (pin >= NUM_DIGITAL_PINS) {
		return false;
	}
	uint8_t port = digitalPinToPort(pin);
	pinReg_ = portInputRegister(port);
	bit_ = digitalPinToBitMask(pin);
	mask_ = ~bit_;
	portReg_ = pinReg_ + 2;
	return true;
}
//------------------------------------------------------------------------------
/** Configure the pin
 *
 * @param[in] mode Configure as output mode if true else input mode.
 * @param[in] data For output mode set pin high if true else low.
 *                 For input mode enable 20K pullup if true else Hi-Z.
 *
 * This function may be used with interrupts enabled or disabled.
 * The previous interrupt state will be restored.
 */
void PinIO::config(bool mode, bool data)
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
		modeI(mode);
		writeI(data);
	}
}
/** @} */
