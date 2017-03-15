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

#include "OMAC.h"
#include "GF128.h"
#include "Crypto.h"
#include <string.h>

/**
 * \class OMAC OMAC.h <OMAC.h>
 * \brief Implementation of the OMAC message authenticator.
 *
 * OMAC is the message authentication part of EAX mode.  It is provided
 * as a separate class for the convenience of applications that need
 * message authentication separate from encryption.
 *
 * References: https://en.wikipedia.org/wiki/EAX_mode,
 * http://web.cs.ucdavis.edu/~rogaway/papers/eax.html
 *
 * \sa EAX
 */

/**
 * \brief Constructs a new OMAC object.
 *
 * This constructor must be followed by a call to setBlockCipher()
 * to specify the block cipher to use.
 */
OMAC::OMAC()
	: _blockCipher(0)
	, posn(0)
{
}

/**
 * \brief Destroys this OMAC object.
 *
 * \sa clear()
 */
OMAC::~OMAC()
{
	clean(b);
}

/**
 * \fn BlockCipher *OMAC::blockCipher() const
 * \brief Gets the block cipher that is in use for this OMAC object.
 *
 * \sa setBlockCipher()
 */

/**
 * \fn void OMAC::setBlockCipher(BlockCipher *cipher)
 * \brief Sets the block cipher to use for this OMAC object.
 *
 * \param cipher The block cipher to use to implement OMAC.
 * This object must have a block size of 128 bits (16 bytes).
 *
 * \sa blockCipher()
 */

/**
 * \brief Initialises the first OMAC hashing context and creates the B value.
 *
 * \param omac The OMAC hashing context.
 *
 * This function must be called first before initNext(), update(), or
 * finalize() to create the B value from the OMAC algorithm which is
 * used to finalize later hashes.  It is assumed that setBlockCipher()
 * has already been called.
 *
 * The tag value for the context is implicitly set to zero, which means
 * that the context can be used for ordinary hashing as long as the
 * data that follows is non-zero in length.  Alternatively, initNext()
 * can be called to restart the context with a specific tag.
 *
 * This function must be called again whenever the block cipher or the
 * key changes.
 *
 * \sa initNext(), update(), finalize()
 */
void OMAC::initFirst(uint8_t omac[16])
{
	// Start the OMAC context.  We assume that the data that follows
	// will be at least 1 byte in length so that we can encrypt the
	// zeroes now to derive the B value.
	memset(omac, 0, 16);
	_blockCipher->encryptBlock(omac, omac);
	posn = 0;

	// Generate the B value from the encrypted block of zeroes.
	// We will need this later when finalising the OMAC hashes.
	memcpy(b, omac, 16);
	GF128::dblEAX(b);
}

/**
 * \brief Initialises or restarts an OMAC hashing context.
 *
 * \param omac The OMAC hashing context.
 * \param tag The tag value indicating which OMAC calculation we are doing.
 *
 * It is assumed that initFirst() was called previously to create the B
 * value for the context.
 *
 * \sa initFirst(), update(), finalize()
 */
void OMAC::initNext(uint8_t omac[16], uint8_t tag)
{
	memset(omac, 0, 15);
	omac[15] = tag;
	posn = 16;
}

/**
 * \brief Updates an OMAC hashing context with more data.
 *
 * \param omac The OMAC hashing context.
 * \param data Points to the data to be hashed.
 * \param size The number of bytes to be hashed.
 *
 * \sa initFirst(), initNext(), finalize()
 */
void OMAC::update(uint8_t omac[16], const uint8_t *data, size_t size)
{
	while (size > 0) {
		// Encrypt the current block if it is already full.
		if (posn == 16) {
			_blockCipher->encryptBlock(omac, omac);
			posn = 0;
		}

		// XOR the incoming data with the current block.
		uint8_t len = 16 - posn;
		if (len > size) {
			len = (uint8_t)size;
		}
		for (uint8_t index = 0; index < len; ++index) {
			omac[posn++] ^= data[index];
		}

		// Move onto the next block.
		size -= len;
		data += len;
	}
}

/**
 * \brief Finalises an OMAC hashing context.
 *
 * \param omac The OMAC hashing context on entry, the final OMAC value on exit.
 *
 * \sa initFirst(), initNext(), update()
 */
void OMAC::finalize(uint8_t omac[16])
{
	// Apply padding if necessary.
	if (posn != 16) {
		// Need padding: XOR with P = 2 * B.
		uint32_t p[4];
		memcpy(p, b, 16);
		GF128::dblEAX(p);
		omac[posn] ^= 0x80;
		for (uint8_t index = 0; index < 16; ++index) {
			omac[index] ^= ((const uint8_t *)p)[index];
		}
		clean(p);
	} else {
		// No padding necessary: XOR with B.
		for (uint8_t index = 0; index < 16; ++index) {
			omac[index] ^= ((const uint8_t *)b)[index];
		}
	}

	// Encrypt the hash to get the final OMAC value.
	_blockCipher->encryptBlock(omac, omac);
}

/**
 * \brief Clears all security-sensitive state from this object.
 */
void OMAC::clear()
{
	clean(b);
}
