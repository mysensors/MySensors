/* Arduino I2C Library
 * Copyright (C) 2013 by William Greiman
 *
 * This file is part of the Arduino I2C Library
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
 * along with the Arduino I2C Library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */
/**
 * @file   I2cConstants.h
 * @brief  Two Wire Interface constants.
 *
 * @defgroup twoWire I2C constants
 * @details  Two Wire Interface library.
 * @{
 */
#ifndef I2cConstants_h
#define I2cConstants_h
#include <inttypes.h>

/** Option argument for transfer() of transferContinue() to continue a
   an I2C operation. */
const uint8_t I2C_CONTINUE =  0;

/** Option argument for transfer() of transferContinue() to end a
    transfer with a STOP condition */
const uint8_t I2C_STOP      = 1;

/** Option argument for transfer() of transferContinue() to end a
    transfer with a repeated START condition */
const uint8_t I2C_REP_START = 2;

/** Set I2C bus speed to 100 kHz. Used by TwiMaster class. */
const uint8_t I2C_100KHZ    = 0;

/** Set I2C bus speed to 400 kHz. Used by TwiMaster class. */
const uint8_t I2C_400KHZ    = 1;

/** Bit to OR with address for a read operation. */
const uint8_t I2C_READ = 1;

/** Bit to OR with address for write operation. */
const uint8_t I2C_WRITE = 0;

/** Disable internal pull-ups on SDA and SCL. Used by TwiMaster class. */
const uint8_t I2C_NO_PULLUPS = 0;

/** Enable internal pull-ups on SDA and SCL. Used by TwiMaster class. */
const uint8_t I2C_INTERNAL_PULLUPS = 1;
#endif  // I2cConstants_h
/** @} */