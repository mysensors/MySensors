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

#include "EAX.h"
#include "Crypto.h"
#include <string.h>

/**
 * \class EAXCommon EAX.h <EAX.h>
 * \brief Concrete base class to assist with implementing EAX for
 * 128-bit block ciphers.
 *
 * References: https://en.wikipedia.org/wiki/EAX_mode,
 * http://web.cs.ucdavis.edu/~rogaway/papers/eax.html
 *
 * \sa EAX
 */

/**
 * \brief Constructs a new cipher in EAX mode.
 *
 * This constructor must be followed by a call to setBlockCipher().
 */
EAXCommon::EAXCommon()
{
	state.encPosn = 0;
	state.authMode = 0;
}

EAXCommon::~EAXCommon()
{
	clean(state);
}

size_t EAXCommon::keySize() const
{
	return omac.blockCipher()->keySize();
}

size_t EAXCommon::ivSize() const
{
	// Can use any size but 16 is recommended.
	return 16;
}

size_t EAXCommon::tagSize() const
{
	// Tags can be up to 16 bytes in length.
	return 16;
}

bool EAXCommon::setKey(const uint8_t *key, size_t len)
{
	return omac.blockCipher()->setKey(key, len);
}

bool EAXCommon::setIV(const uint8_t *iv, size_t len)
{
	// Must have at least 1 byte for the IV.
	if (!len) {
		return false;
	}

	// Hash the IV to create the initial nonce for CTR mode.  Also creates B.
	omac.initFirst(state.counter);
	omac.update(state.counter, iv, len);
	omac.finalize(state.counter);

	// The tag is initially the nonce value.  Will be XOR'ed with
	// the hash of the authenticated and encrypted data later.
	memcpy(state.tag, state.counter, 16);

	// Start the hashing context for the authenticated data.
	omac.initNext(state.hash, 1);
	state.encPosn = 16;
	state.authMode = 1;

	// The EAX context is ready to go.
	return true;
}

void EAXCommon::encrypt(uint8_t *output, const uint8_t *input, size_t len)
{
	if (state.authMode) {
		closeAuthData();
	}
	encryptCTR(output, input, len);
	omac.update(state.hash, output, len);
}

void EAXCommon::decrypt(uint8_t *output, const uint8_t *input, size_t len)
{
	if (state.authMode) {
		closeAuthData();
	}
	omac.update(state.hash, input, len);
	encryptCTR(output, input, len);
}

void EAXCommon::addAuthData(const void *data, size_t len)
{
	if (state.authMode) {
		omac.update(state.hash, (const uint8_t *)data, len);
	}
}

void EAXCommon::computeTag(void *tag, size_t len)
{
	closeTag();
	if (len > 16) {
		len = 16;
	}
	memcpy(tag, state.tag, len);
}

bool EAXCommon::checkTag(const void *tag, size_t len)
{
	// Can never match if the expected tag length is too long.
	if (len > 16) {
		return false;
	}

	// Compute the final tag and check it.
	closeTag();
	return secure_compare(state.tag, tag, len);
}

void EAXCommon::clear()
{
	clean(state);
}

/**
 * \brief Closes the authenticated data portion of the session and
 * starts encryption or decryption.
 */
void EAXCommon::closeAuthData()
{
	// Finalise the OMAC hash and XOR it with the final tag.
	omac.finalize(state.hash);
	for (uint8_t index = 0; index < 16; ++index) {
		state.tag[index] ^= state.hash[index];
	}
	state.authMode = 0;

	// Initialise the hashing context for the ciphertext data.
	omac.initNext(state.hash, 2);
}

/**
 * \brief Encrypts or decrypts a region using the block cipher in CTR mode.
 *
 * \param output The output buffer to write to, which may be the same
 * buffer as \a input.  The \a output buffer must have at least as many
 * bytes as the \a input buffer.
 * \param input The input buffer to read from.
 * \param len The number of bytes to process.
 */
void EAXCommon::encryptCTR(uint8_t *output, const uint8_t *input, size_t len)
{
	while (len > 0) {
		// Do we need to start a new block?
		if (state.encPosn == 16) {
			// Encrypt the counter to create the next keystream block.
			omac.blockCipher()->encryptBlock(state.stream, state.counter);
			state.encPosn = 0;

			// Increment the counter, taking care not to reveal
			// any timing information about the starting value.
			// We iterate through the entire counter region even
			// if we could stop earlier because a byte is non-zero.
			uint16_t temp = 1;
			uint8_t index = 16;
			while (index > 0) {
				--index;
				temp += state.counter[index];
				state.counter[index] = (uint8_t)temp;
				temp >>= 8;
			}
		}

		// Encrypt/decrypt the current input block.
		uint8_t size = 16 - state.encPosn;
		if (size > len) {
			size = (uint8_t)len;
		}
		for (uint8_t index = 0; index < size; ++index) {
			output[index] = input[index] ^ state.stream[(state.encPosn)++];
		}

		// Move onto the next block.
		len -= size;
		input += size;
		output += size;
	}
}

void EAXCommon::closeTag()
{
	// If we were only authenticating, then close off auth mode.
	if (state.authMode) {
		closeAuthData();
	}

	// Finalise the hash over the ciphertext and XOR with the final tag.
	omac.finalize(state.hash);
	for (uint8_t index = 0; index < 16; ++index) {
		state.tag[index] ^= state.hash[index];
	}
}

/**
 * \fn void EAXCommon::setBlockCipher(BlockCipher *cipher)
 * \brief Sets the block cipher to use for this EAX object.
 *
 * \param cipher The block cipher to use to implement EAX mode.
 * This object must have a block size of 128 bits (16 bytes).
 */

/**
 * \class EAX EAX.h <EAX.h>
 * \brief Implementation of the EAX authenticated cipher.
 *
 * EAX mode converts a block cipher into an authenticated cipher
 * that uses the block cipher T to encrypt and authenticate.
 *
 * The size of the key is determined by the underlying block cipher T.
 * The IV is recommended to be 128 bits (16 bytes) in length, but other
 * lengths are supported as well.  The default tagSize() is 128 bits
 * (16 bytes) but the EAX specification does allow smaller tag sizes.
 *
 * The template parameter T must be a concrete subclass of BlockCipher
 * indicating the specific block cipher to use.  The block cipher must
 * have a block size of 128 bits.  For example, the following creates a
 * EAX object using AES256 as the underlying cipher and then uses it
 * to encrypt and authenticate a \c plaintext block:
 *
 * \code
 * EAX<AES256> eax;
 * eax.setKey(key, sizeof(key));
 * eax.setIV(iv, sizeof(iv));
 * eax.addAuthData(adata, sizeof(adata));
 * eax.encrypt(ciphertext, plaintext, sizeof(plaintext));
 * eax.computeTag(tag, sizeof(tag));
 * \endcode
 *
 * The decryption process is almost identical to convert a \c ciphertext and
 * \a tag back into plaintext and then check the tag:
 *
 * \code
 * EAX<AES256> eax;
 * eax.setKey(key, sizeof(key));
 * eax.setIV(iv, sizeof(iv));
 * eax.addAuthData(adata, sizeof(adata));
 * eax.decrypt(ciphertext, plaintext, sizeof(plaintext));
 * if (!eax.checkTag(tag, sizeof(tag))) {
 *     // The data was invalid - do not use it.
 *     ...
 * }
 * \endcode
 *
 * The EAX class can also be used to implement message authentication
 * by omitting the plaintext:
 *
 * \code
 * EAX<AES256> eax;
 * eax.setKey(key, sizeof(key));
 * eax.setIV(iv, sizeof(iv));
 * eax.addAuthData(adata1, sizeof(adata1));
 * eax.addAuthData(adata2, sizeof(adata1));
 * ...
 * eax.addAuthData(adataN, sizeof(adataN));
 * eax.computeTag(tag, sizeof(tag));
 * \endcode
 *
 * References: https://en.wikipedia.org/wiki/EAX_mode,
 * http://web.cs.ucdavis.edu/~rogaway/papers/eax.html
 *
 * \sa EAXCommon, GCM
 */

/**
 * \fn EAX::EAX()
 * \brief Constructs a new EAX object for the block cipher T.
 */
