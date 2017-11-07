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
 * @ingroup internals
 * @details  Two Wire Interface library.
 * @{
 */
#ifndef PinIO_h
#define PinIO_h
#include <util/atomic.h>
#include <avr/io.h>
/** @brief Helper macro for complex inline attributes */
#define ALWAYS_INLINE inline __attribute__((always_inline))
//------------------------------------------------------------------------------
/**
 * @class PinIO
 * @brief AVR port I/O with runtime pin numbers.
 */
class PinIO
{
public:
	/** Create a PinIO object with no assigned pin. */
	// Suppress warning about uninitialized variables because initializing them in an init function
	// allows the compiler to optimize away the variables in case the class is only instantiated but
	// never used.
	// cppcheck-suppress uninitMemberVar
	PinIO() : bit_(0), mask_(0XFF) {}
	/** Constructor
	 * @param[in] pin Pin assigned to this object.
	 */
	explicit PinIO(uint8_t pin);
	/** Initialize pin bit mask and port address.
	 * @param[in] pin Arduino board pin number.
	 * @return true for success or false if invalid pin number.
	 */
	bool begin(uint8_t pin);
	/** Configure the pin
	 *
	 * @param[in] mode Configure as output mode if true else input mode.
	 * @param[in] data For output mode set pin high if true else low.
	 *                 For input mode enable 20K pullup if true else Hi-Z.
	 *
	 * This function may be used with interrupts enabled or disabled.
	 * The previous interrupt state will be restored.
	 */
	void config(bool mode, bool data);
	//----------------------------------------------------------------------------
	/** @return Pin's level */
	ALWAYS_INLINE bool read()
	{
		return *pinReg_ & bit_;
	}
	//----------------------------------------------------------------------------
	/** toggle a pin
	 *
	 * If the pin is in output mode toggle the pin's level.
	 * If the pin is in input mode toggle the state of the 20K pullup.
	 */
	ALWAYS_INLINE void toggle()
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
	ALWAYS_INLINE void highI()
	{
		writeI(1);
	}
	/**
	 * Set pin low if output mode or disable 20K pullup if input mode.
	 *
	 * This function must be called with interrupts disabled.
	 * This function will not change the interrupt state.
	 */
	ALWAYS_INLINE void lowI()
	{
		writeI(0);
	}
	/**
	 * Set pin mode
	 * @param[in] mode if true set output mode else input mode.
	 *
	 * mode() does not enable or disable the 20K pullup for input mode.
	 *
	 * This function must be called with interrupts disabled.
	 * This function will not change the interrupt state.
	 */
	ALWAYS_INLINE void modeI(bool mode)
	{
		volatile uint8_t* ddrReg = pinReg_ + 1;
		*ddrReg = mode ? *ddrReg | bit_ : *ddrReg & mask_;
	}
	/**  Write pin.
	 *
	 * @param[in] level If output mode set pin high if true else low.
	 * If input mode enable 20K pullup if true else disable pullup.
	 *
	 * This function must be called with interrupts disabled.
	 * This function will not change the interrupt state.
	 */
	ALWAYS_INLINE void writeI(bool level)
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
	ALWAYS_INLINE void high()
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
	ALWAYS_INLINE void low()
	{
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			lowI();
		}
	}
	/**
	 * Set pin mode
	 * @param[in] pinMode if true set output mode else input mode.
	 *
	 * mode() does not enable or disable the 20K pullup for input mode.
	 *
	 * This function will enable interrupts.  This function should not be
	 * called in an ISR or where interrupts are disabled.
	 */
	ALWAYS_INLINE void mode(bool pinMode)
	{
		ATOMIC_BLOCK(ATOMIC_FORCEON) {
			modeI(pinMode);
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
	ALWAYS_INLINE void write(bool level)
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
#endif  // PinIO_h
/** @} */
