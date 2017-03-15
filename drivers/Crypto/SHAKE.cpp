/*
 * Copyright (C) 2016 Southern Storm Software, Pty Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "SHAKE.h"

/**
 * \class SHAKE SHAKE.h <SHAKE.h>
 * \brief Abstract base class for the SHAKE Extendable-Output Functions (XOFs).
 *
 * Reference: http://en.wikipedia.org/wiki/SHA-3
 *
 * \sa SHAKE256, SHAKE128, SHA3_256
 */

/**
 * \brief Constructs a SHAKE object.
 *
 * \param capacity The capacity of the Keccak sponge function in bits which
 * should be a multiple of 64 and between 64 and 1536.
 */
SHAKE::SHAKE(size_t capacity)
	: finalized(false)
{
	core.setCapacity(capacity);
}

/**
 * \brief Destroys this SHAKE object after clearing all sensitive information.
 */
SHAKE::~SHAKE()
{
}

size_t SHAKE::blockSize() const
{
	return core.blockSize();
}

void SHAKE::reset()
{
	core.reset();
	finalized = false;
}

void SHAKE::update(const void *data, size_t len)
{
	if (finalized) {
		reset();
	}
	core.update(data, len);
}

void SHAKE::extend(uint8_t *data, size_t len)
{
	if (!finalized) {
		core.pad(0x1F);
		finalized = true;
	}
	core.extract(data, len);
}

void SHAKE::encrypt(uint8_t *output, const uint8_t *input, size_t len)
{
	if (!finalized) {
		core.pad(0x1F);
		finalized = true;
	}
	core.encrypt(output, input, len);
}

void SHAKE::clear()
{
	core.clear();
	finalized = false;
}

/**
 * \class SHAKE128 SHAKE.h <SHAKE.h>
 * \brief SHAKE Extendable-Output Function (XOF) with 128-bit security.
 *
 * Reference: http://en.wikipedia.org/wiki/SHA-3
 *
 * \sa SHAKE256, SHAKE, SHA3_256
 */

/**
 * \fn SHAKE128::SHAKE128()
 * \brief Constructs a SHAKE object with 128-bit security.
 */

/**
 * \brief Destroys this SHAKE128 object after clearing all sensitive
 * information.
 */
SHAKE128::~SHAKE128()
{
}

/**
 * \class SHAKE256 SHAKE.h <SHAKE.h>
 * \brief SHAKE Extendable-Output Function (XOF) with 256-bit security.
 *
 * Reference: http://en.wikipedia.org/wiki/SHA-3
 *
 * \sa SHAKE128, SHAKE, SHA3_256
 */

/**
 * \fn SHAKE256::SHAKE256()
 * \brief Constructs a SHAKE object with 256-bit security.
 */

/**
 * \brief Destroys this SHAKE256 object after clearing all sensitive
 * information.
 */
SHAKE256::~SHAKE256()
{
}
