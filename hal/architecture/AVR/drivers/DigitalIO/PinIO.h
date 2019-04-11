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
#ifndef PinIO_h
#define PinIO_h
#if defined(__AVR__) || defined(DOXYGEN)  // AVR only
#include <Arduino.h>
#include <util/atomic.h>
#include <avr/io.h>
//------------------------------------------------------------------------------
/**
 * @class PinIO
 * @brief AVR port I/O with runtime pin numbers.
 */
class PinIO
{
public:
	/** Create a PinIO object with no assigned pin. */
	// cppcheck-suppress uninitMemberVar
	PinIO() : bit_(0), mask_(0XFF) {}
	explicit PinIO(uint8_t pin);
	bool begin(uint8_t pin);
	void config(uint8_t mode, bool data);
	//----------------------------------------------------------------------------
	/** @return Pin's level */
	inline __attribute__((always_inline))
	bool read()
	{
		return *pinReg_ & bit_;
	}
	//----------------------------------------------------------------------------
	/** toggle a pin
	 *
	 * If the pin is in output mode toggle the pin's level.
	 * If the pin is in input mode toggle the state of the 20K pullup.
	 */
	inline __attribute__((always_inline))
	void toggle()
	{
		*pinReg_ = bit_;
	}
	//============================================================================
	/**
	 * Set pin high if output mode or enable 20K pullup if input mode.
	 *
	 * This function must be called with interrupts disabled.
	 * This function will not change the interrupt state.
	 */
	inline __attribute__((always_inline))
	void highI()
	{
		writeI(1);
	}
	/**
	 * Set pin low if output mode or disable 20K pullup if input mode.
	 *
	 * This function must be called with interrupts disabled.
	 * This function will not change the interrupt state.
	 */
	inline __attribute__((always_inline))
	void lowI()
	{
		writeI(0);
	}
	/** Set pin mode.
	 *
	 * @param[in] mode: INPUT, OUTPUT, or INPUT_PULLUP.
	 *
	 * The internal pullup resistors will be enabled if mode is INPUT_PULLUP
	 * and disabled if the mode is INPUT.
	 *
	 * This function must be called with interrupts disabled.
	 * This function will not change the interrupt state.
	 */
	inline __attribute__((always_inline))
	void modeI(uint8_t mode)
	{
		volatile uint8_t* ddrReg = pinReg_ + 1;
		*ddrReg = mode == OUTPUT ? *ddrReg | bit_ : *ddrReg & mask_;
		if (mode != OUTPUT) {
			writeI(mode == INPUT_PULLUP);
		}
	}

	/**  Write pin.
	 *
	 * @param[in] level If output mode set pin high if true else low.
	 * If input mode enable 20K pullup if true else disable pullup.
	 *
	 * This function must be called with interrupts disabled.
	 * This function will not change the interrupt state.
	 */
	inline __attribute__((always_inline))
	void writeI(bool level)
	{
		*portReg_ = level ? *portReg_ | bit_ : *portReg_ & mask_;
	}
	//============================================================================
	/**
	 * Set pin level high if output mode or enable 20K pullup if input mode.
	 *
	 * This function will enable interrupts.  This function should not be
	 * called in an ISR or where interrupts are disabled.
	 */
	inline __attribute__((always_inline))
	void high()
	{
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			highI();
		}
	}

	/**
	 * Set pin level low if output mode or disable 20K pullup if input mode.
	 *
	 * This function will enable interrupts.  This function should not be
	 * called in an ISR or where interrupts are disabled.
	 */
	inline __attribute__((always_inline))
	void low()
	{
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			lowI();
		}
	}

	/**
	 * Set pin mode.
	 *
	 * @param[in] mode: INPUT, OUTPUT, or INPUT_PULLUP.
	 *
	 * The internal pullup resistors will be enabled if mode is INPUT_PULLUP
	 * and disabled if the mode is INPUT.
	 *
	 * This function will enable interrupts.  This function should not be
	 * called in an ISR or where interrupts are disabled.
	 */
	inline __attribute__((always_inline))
	void mode(uint8_t mode)
	{
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			modeI(mode);
		}
	}

	/**  Write pin.
	 *
	 * @param[in] level If output mode set pin high if true else low.
	 * If input mode enable 20K pullup if true else disable pullup.
	 *
	 * This function will enable interrupts.  This function should not be
	 * called in an ISR or where interrupts are disabled.
	 */
	inline __attribute__((always_inline))
	void write(bool level)
	{
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			writeI(level);
		}
	}
	//----------------------------------------------------------------------------
private:
	uint8_t bit_;
	uint8_t mask_;
	volatile uint8_t* pinReg_;
	volatile uint8_t* portReg_;
};
#endif  // __AVR__
#endif  // PinIO_h
/** @} */
