/*
 * Copyright (C) 2015 Southern Storm Software, Pty Ltd.
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

#include "SHA3.h"
#include "Crypto.h"

/**
 * \class SHA3_256 SHA3.h <SHA3.h>
 * \brief SHA3-256 hash algorithm.
 *
 * Reference: http://en.wikipedia.org/wiki/SHA-3
 *
 * \sa SHA3_512
 */

/**
 * \brief Constructs a new SHA3-256 hash object.
 */
SHA3_256::SHA3_256()
{
	core.setCapacity(512);
}

/**
 * \brief Destroys this hash object after clearing sensitive information.
 */
SHA3_256::~SHA3_256()
{
	// The destructor for the KeccakCore object will do most of the work.
}

size_t SHA3_256::hashSize() const
{
	return 32;
}

size_t SHA3_256::blockSize() const
{
	return core.blockSize();
}

void SHA3_256::reset()
{
	core.reset();
}

void SHA3_256::update(const void *data, size_t len)
{
	core.update(data, len);
}

void SHA3_256::finalize(void *hash, size_t len)
{
	// Pad the final block and then extract the hash value.
	core.pad(0x06);
	core.extract(hash, len);
}

void SHA3_256::clear()
{
	core.clear();
}

void SHA3_256::resetHMAC(const void *key, size_t keyLen)
{
	core.setHMACKey(key, keyLen, 0x36, 32);
}

void SHA3_256::finalizeHMAC(const void *key, size_t keyLen, void *hash, size_t hashLen)
{
	uint8_t temp[32];
	finalize(temp, sizeof(temp));
	core.setHMACKey(key, keyLen, 0x5C, 32);
	core.update(temp, sizeof(temp));
	finalize(hash, hashLen);
	clean(temp);
}

/**
 * \class SHA3_512 SHA3.h <SHA3.h>
 * \brief SHA3-512 hash algorithm.
 *
 * Reference: http://en.wikipedia.org/wiki/SHA-3
 *
 * \sa SHA3_256
 */

/**
 * \brief Constructs a new SHA3-512 hash object.
 */
SHA3_512::SHA3_512()
{
	core.setCapacity(1024);
}

/**
 * \brief Destroys this hash object after clearing sensitive information.
 */
SHA3_512::~SHA3_512()
{
	// The destructor for the KeccakCore object will do most of the work.
}

size_t SHA3_512::hashSize() const
{
	return 64;
}

size_t SHA3_512::blockSize() const
{
	return core.blockSize();
}

void SHA3_512::reset()
{
	core.reset();
}

void SHA3_512::update(const void *data, size_t len)
{
	core.update(data, len);
}

void SHA3_512::finalize(void *hash, size_t len)
{
	// Pad the final block and then extract the hash value.
	core.pad(0x06);
	core.extract(hash, len);
}

void SHA3_512::clear()
{
	core.clear();
}

void SHA3_512::resetHMAC(const void *key, size_t keyLen)
{
	core.setHMACKey(key, keyLen, 0x36, 64);
}

void SHA3_512::finalizeHMAC(const void *key, size_t keyLen, void *hash, size_t hashLen)
{
	uint8_t temp[64];
	finalize(temp, sizeof(temp));
	core.setHMACKey(key, keyLen, 0x5C, 64);
	core.update(temp, sizeof(temp));
	finalize(hash, hashLen);
	clean(temp);
}
