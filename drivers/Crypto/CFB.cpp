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

#include "CFB.h"
#include "Crypto.h"
#include <string.h>

/**
 * \class CFBCommon CFB.h <CFB.h>
 * \brief Concrete base class to assist with implementing CFB for
 * 128-bit block ciphers.
 *
 * Reference: http://en.wikipedia.org/wiki/Block_cipher_mode_of_operation
 *
 * \sa CFB
 */

/**
 * \brief Constructs a new cipher in CFB mode.
 *
 * This constructor should be followed by a call to setBlockCipher().
 */
CFBCommon::CFBCommon()
	: blockCipher(0)
	, posn(16)
{
}

/**
 * \brief Destroys this cipher object after clearing sensitive information.
 */
CFBCommon::~CFBCommon()
{
	clean(iv);
}

size_t CFBCommon::keySize() const
{
	return blockCipher->keySize();
}

size_t CFBCommon::ivSize() const
{
	return 16;
}

bool CFBCommon::setKey(const uint8_t *key, size_t len)
{
	// Verify the cipher's block size, just in case.
	if (blockCipher->blockSize() != 16) {
		return false;
	}

	// Set the key on the underlying block cipher.
	return blockCipher->setKey(key, len);
}

bool CFBCommon::setIV(const uint8_t *iv, size_t len)
{
	if (len != 16) {
		return false;
	}
	memcpy(this->iv, iv, 16);
	posn = 16;
	return true;
}

void CFBCommon::encrypt(uint8_t *output, const uint8_t *input, size_t len)
{
	uint8_t size;
	while (len > 0) {
		// If we have exhausted the current keystream block, then encrypt
		// the IV/ciphertext to get another keystream block.
		if (posn >= 16) {
			blockCipher->encryptBlock(iv, iv);
			posn = 0;
		}

		// XOR the plaintext with the encrypted IV to get the new ciphertext.
		// We keep building up the ciphertext byte by byte in the IV buffer
		// until we have a full block's worth, and then the IV is encrypted
		// again by the code above.
		size = 16 - posn;
		if (size > len) {
			size = len;
		}
		len -= size;
		while (size > 0) {
			iv[posn] ^= *input++;
			*output++ = iv[posn++];
			--size;
		}
	}
}

void CFBCommon::decrypt(uint8_t *output, const uint8_t *input, size_t len)
{
	uint8_t size;
	while (len > 0) {
		// If we have exhausted the current keystream block, then encrypt
		// the IV/ciphertext to get another keystream block.
		if (posn >= 16) {
			blockCipher->encryptBlock(iv, iv);
			posn = 0;
		}

		// XOR the ciphertext with the encrypted IV to get the new plaintext.
		// We keep building up the ciphertext byte by byte in the IV buffer
		// until we have a full block's worth, and then the IV is encrypted
		// again by the code above.
		size = 16 - posn;
		if (size > len) {
			size = len;
		}
		len -= size;
		while (size > 0) {
			uint8_t in = *input++;
			*output++ = iv[posn] ^ in;
			iv[posn++] = in;
			--size;
		}
	}
}

void CFBCommon::clear()
{
	blockCipher->clear();
	clean(iv);
	posn = 16;
}

/**
 * \fn void CFBCommon::setBlockCipher(BlockCipher *cipher)
 * \brief Sets the block cipher to use for this CFB object.
 *
 * \param cipher The block cipher to use to implement CFB mode,
 * which must have a block size of 16 bytes (128 bits).
 */

/**
 * \class CFB CFB.h <CFB.h>
 * \brief Implementation of the Cipher Feedback (CFB) mode for
 * 128-bit block ciphers.
 *
 * The template parameter T must be a concrete subclass of BlockCipher
 * indicating the specific block cipher to use.  T must have a block size
 * of 16 bytes (128 bits).  The size of the CFB shift register is the same
 * as the block size.
 *
 * For example, the following creates a CFB object using AES192 as the
 * underlying cipher:
 *
 * \code
 * CFB<AES192> cfb;
 * cfb.setKey(key, 24);
 * cfb.setIV(iv, 16);
 * cfb.encrypt(output, input, len);
 * \endcode
 *
 * Decryption is similar:
 *
 * \code
 * CFB<AES192> cfb;
 * cfb.setKey(key, 24);
 * cfb.setIV(iv, 16);
 * cfb.decrypt(output, input, len);
 * \endcode
 *
 * The size of the ciphertext will always be the same as the size of
 * the plaintext.
 *
 * Reference: http://en.wikipedia.org/wiki/Block_cipher_mode_of_operation
 *
 * \sa CTR, OFB, CBC
 */

/**
 * \fn CFB::CFB()
 * \brief Constructs a new CFB object for the block cipher T.
 */
