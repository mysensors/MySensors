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

#include "Poly1305.h"
#include "Crypto.h"
#include "utility/EndianUtil.h"
#include "utility/LimbUtil.h"
#include <string.h>

/**
 * \class Poly1305 Poly1305.h <Poly1305.h>
 * \brief Poly1305 message authenticator
 *
 * Poly1305 is a message authenticator designed by Daniel J. Bernstein.
 * An arbitrary-length message is broken up into 16-byte chunks and fed
 * into a polynomial mod 2<sup>130</sup> - 5 based on the 16-byte
 * authentication key.  The final polynomial value is then combined with a
 * 16-byte nonce to create the authentication token.
 *
 * The following example demonstrates how to compute an authentication token
 * for a message made up of several blocks under a specific key and nonce:
 *
 * \code
 * Poly1305 poly1305;
 * uint8_t token[16];
 * poly1305.reset(key);
 * poly1305.update(block1, sizeof(block1));
 * poly1305.update(block2, sizeof(block2));
 * ...
 * poly1305.update(blockN, sizeof(blockN));
 * poly1305.finalize(nonce, token, sizeof(token));
 * \endcode
 *
 * In the original Poly1305 specification, the nonce was encrypted with AES
 * and a second 16-byte key.  Since then, common practice has been for the
 * caller to encrypt the nonce which gives the caller more flexibility as
 * to how to derive and/or encrypt the nonce.
 *
 * References: http://en.wikipedia.org/wiki/Poly1305-AES,
 * http://cr.yp.to/mac.html
 */

// Limb array with enough space for 130 bits.
#define NUM_LIMBS_130BIT    (NUM_LIMBS_128BIT + 1)

// Endian helper macros for limbs and arrays of limbs.
#if BIGNUMBER_LIMB_8BIT
#define lelimbtoh(x)        (x)
#define htolelimb(x)        (x)
#elif BIGNUMBER_LIMB_16BIT
#define lelimbtoh(x)        (le16toh((x)))
#define htolelimb(x)        (htole16((x)))
#elif BIGNUMBER_LIMB_32BIT
#define lelimbtoh(x)        (le32toh((x)))
#define htolelimb(x)        (htole32((x)))
#elif BIGNUMBER_LIMB_64BIT
#define lelimbtoh(x)        (le64toh((x)))
#define htolelimb(x)        (htole64((x)))
#endif
#if defined(CRYPTO_LITTLE_ENDIAN)
#define littleToHost(r,size)    do { ; } while (0)
#else
#define littleToHost(r,size)   \
	do { \
		for (uint8_t i = 0; i < (size); ++i) \
			(r)[i] = lelimbtoh((r)[i]); \
	} while (0)
#endif

/**
 * \brief Constructs a new Poly1305 message authenticator.
 */
Poly1305::Poly1305()
{
	state.chunkSize = 0;
}

/**
 * \brief Destroys this Poly1305 message authenticator after clearing all
 * sensitive information.
 */
Poly1305::~Poly1305()
{
	clean(state);
}

/**
 * \brief Resets the Poly1305 message authenticator for a new session.
 *
 * \param key Points to the 16 byte authentication key.
 *
 * \sa update(), finalize()
 */
void Poly1305::reset(const void *key)
{
	// Copy the key into place and clear the bits we don't need.
	uint8_t *r = (uint8_t *)state.r;
	memcpy(r, key, 16);
	r[3] &= 0x0F;
	r[4] &= 0xFC;
	r[7] &= 0x0F;
	r[8] &= 0xFC;
	r[11] &= 0x0F;
	r[12] &= 0xFC;
	r[15] &= 0x0F;

	// Convert into little-endian if necessary.
	littleToHost(state.r, NUM_LIMBS_128BIT);

	// Reset the hashing process.
	state.chunkSize = 0;
	memset(state.h, 0, sizeof(state.h));
}

/**
 * \brief Updates the message authenticator with more data.
 *
 * \param data Data to be hashed.
 * \param len Number of bytes of data to be hashed.
 *
 * If finalize() has already been called, then the behavior of update() will
 * be undefined.  Call reset() first to start a new authentication process.
 *
 * \sa pad(), reset(), finalize()
 */
void Poly1305::update(const void *data, size_t len)
{
	// Break the input up into 128-bit chunks and process each in turn.
	const uint8_t *d = (const uint8_t *)data;
	while (len > 0) {
		uint8_t size = 16 - state.chunkSize;
		if (size > len) {
			size = len;
		}
		memcpy(((uint8_t *)state.c) + state.chunkSize, d, size);
		state.chunkSize += size;
		len -= size;
		d += size;
		if (state.chunkSize == 16) {
			littleToHost(state.c, NUM_LIMBS_128BIT);
			state.c[NUM_LIMBS_128BIT] = 1;
			processChunk();
			state.chunkSize = 0;
		}
	}
}

/**
 * \brief Finalizes the authentication process and returns the token.
 *
 * \param nonce Points to the 16-bit nonce to combine with the token.
 * \param token The buffer to return the token value in.
 * \param len The length of the \a token buffer between 0 and 16.
 *
 * If \a len is less than 16, then the token value will be truncated to
 * the first \a len bytes.  If \a len is greater than 16, then the remaining
 * bytes will left unchanged.
 *
 * If finalize() is called again, then the returned \a token value is
 * undefined.  Call reset() first to start a new authentication process.
 *
 * \sa reset(), update()
 */
void Poly1305::finalize(const void *nonce, void *token, size_t len)
{
	dlimb_t carry;
	uint8_t i;
	limb_t t[NUM_LIMBS_256BIT + 1];

	// Pad and flush the final chunk.
	if (state.chunkSize > 0) {
		uint8_t *c = (uint8_t *)state.c;
		c[state.chunkSize] = 1;
		memset(c + state.chunkSize + 1, 0, 16 - state.chunkSize - 1);
		littleToHost(state.c, NUM_LIMBS_128BIT);
		state.c[NUM_LIMBS_128BIT] = 0;
		processChunk();
	}

	// At this point, processChunk() has left h as a partially reduced
	// result that is less than (2^130 - 5) * 6.  Perform one more
	// reduction and a trial subtraction to produce the final result.

	// Multiply the high bits of h by 5 and add them to the 130 low bits.
	carry = (dlimb_t)((state.h[NUM_LIMBS_128BIT] >> 2) +
	                  (state.h[NUM_LIMBS_128BIT] & ~((limb_t)3)));
	state.h[NUM_LIMBS_128BIT] &= 0x0003;
	for (i = 0; i < NUM_LIMBS_128BIT; ++i) {
		carry += state.h[i];
		state.h[i] = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	state.h[i] += (limb_t)carry;

	// Subtract (2^130 - 5) from h by computing t = h + 5 - 2^130.
	// The "minus 2^130" step is implicit.
	carry = 5;
	for (i = 0; i < NUM_LIMBS_130BIT; ++i) {
		carry += state.h[i];
		t[i] = (limb_t)carry;
		carry >>= LIMB_BITS;
	}

	// Borrow occurs if bit 2^130 of the previous t result is zero.
	// Carefully turn this into a selection mask so we can select either
	// h or t as the final result.  We don't care about the highest word
	// of the result because we are about to drop it in the next step.
	// We have to do it this way to avoid giving away any information
	// about the value of h in the instruction timing.
	limb_t mask = (~((t[NUM_LIMBS_128BIT] >> 2) & 1)) + 1;
	limb_t nmask = ~mask;
	for (i = 0; i < NUM_LIMBS_128BIT; ++i) {
		state.h[i] = (state.h[i] & nmask) | (t[i] & mask);
	}

	// Add the encrypted nonce and format the final hash.
	memcpy(state.c, nonce, 16);
	littleToHost(state.c, NUM_LIMBS_128BIT);
	carry = 0;
	for (i = 0; i < NUM_LIMBS_128BIT; ++i) {
		carry += state.h[i];
		carry += state.c[i];
		state.h[i] = htolelimb((limb_t)carry);
		carry >>= LIMB_BITS;
	}
	if (len > 16) {
		len = 16;
	}
	memcpy(token, state.h, len);
}

/**
 * \brief Pads the input stream with zero bytes to a multiple of 16.
 *
 * \sa update()
 */
void Poly1305::pad()
{
	if (state.chunkSize != 0) {
		memset(((uint8_t *)state.c) + state.chunkSize, 0, 16 - state.chunkSize);
		littleToHost(state.c, NUM_LIMBS_128BIT);
		state.c[NUM_LIMBS_128BIT] = 1;
		processChunk();
		state.chunkSize = 0;
	}
}

/**
 * \brief Clears the authenticator's state, removing all sensitive data.
 */
void Poly1305::clear()
{
	clean(state);
}

/**
 * \brief Processes a single 128-bit chunk of input data.
 */
void Poly1305::processChunk()
{
	limb_t t[NUM_LIMBS_256BIT + 1];

	// Compute h = ((h + c) * r) mod (2^130 - 5).

	// Start with h += c.  We assume that h is less than (2^130 - 5) * 6
	// and that c is less than 2^129, so the result will be less than 2^133.
	dlimb_t carry = 0;
	uint8_t i, j;
	for (i = 0; i < NUM_LIMBS_130BIT; ++i) {
		carry += state.h[i];
		carry += state.c[i];
		state.h[i] = (limb_t)carry;
		carry >>= LIMB_BITS;
	}

	// Multiply h by r.  We know that r is less than 2^124 because the
	// top 4 bits were AND-ed off by reset().  That makes h * r less
	// than 2^257.  Which is less than the (2^130 - 6)^2 we want for
	// the modulo reduction step that follows.
	carry = 0;
	limb_t word = state.r[0];
	for (i = 0; i < NUM_LIMBS_130BIT; ++i) {
		carry += ((dlimb_t)(state.h[i])) * word;
		t[i] = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	t[NUM_LIMBS_130BIT] = (limb_t)carry;
	for (i = 1; i < NUM_LIMBS_128BIT; ++i) {
		word = state.r[i];
		carry = 0;
		for (j = 0; j < NUM_LIMBS_130BIT; ++j) {
			carry += ((dlimb_t)(state.h[j])) * word;
			carry += t[i + j];
			t[i + j] = (limb_t)carry;
			carry >>= LIMB_BITS;
		}
		t[i + NUM_LIMBS_130BIT] = (limb_t)carry;
	}

	// Reduce h * r modulo (2^130 - 5) by multiplying the high 130 bits by 5
	// and adding them to the low 130 bits.  See the explaination in the
	// comments for Curve25519::reduce() for a description of how this works.
	carry = ((dlimb_t)(t[NUM_LIMBS_128BIT] >> 2)) +
	        (t[NUM_LIMBS_128BIT] & ~((limb_t)3));
	t[NUM_LIMBS_128BIT] &= 0x0003;
	for (i = 0; i < NUM_LIMBS_128BIT; ++i) {
		// Shift the next word of t up by (LIMB_BITS - 2) bits and then
		// multiply it by 5.  Breaking it down, we can add the results
		// of shifting up by LIMB_BITS and shifting up by (LIMB_BITS - 2).
		// The main wrinkle here is that this can result in an intermediate
		// carry that is (LIMB_BITS * 2 + 1) bits in size which doesn't
		// fit within a dlimb_t variable.  However, we can defer adding
		// (word << LIMB_BITS) until after the "carry >>= LIMB_BITS" step
		// because it won't affect the low bits of the carry.
		word = t[i + NUM_LIMBS_130BIT];
		carry += ((dlimb_t)word) << (LIMB_BITS - 2);
		carry += t[i];
		state.h[i] = (limb_t)carry;
		carry >>= LIMB_BITS;
		carry += word;
	}
	state.h[i] = (limb_t)(carry + t[NUM_LIMBS_128BIT]);

	// At this point, h is either the answer of reducing modulo (2^130 - 5)
	// or it is at most 5 subtractions away from the answer we want.
	// Leave it as-is for now with h less than (2^130 - 5) * 6.  It is
	// still within a range where the next h * r step will not overflow.
}
