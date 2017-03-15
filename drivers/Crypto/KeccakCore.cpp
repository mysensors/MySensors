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

#include "KeccakCore.h"
#include "Crypto.h"
#include "utility/EndianUtil.h"
#include "utility/RotateUtil.h"
#include "utility/ProgMemUtil.h"
#include <string.h>

/**
 * \class KeccakCore KeccakCore.h <KeccakCore.h>
 * \brief Keccak core sponge function.
 *
 * KeccakCore provides the core sponge function for different capacities.
 * It is used to implement algorithms such as SHA3 and SHAKE.
 *
 * References: http://en.wikipedia.org/wiki/SHA-3
 *
 * \sa SHA3_256, SHAKE256
 */

#if !defined(CRYPTO_LITTLE_ENDIAN)
// All of the Arduino platforms we care about are little-endian.
#error "KeccakCore is not supported on big-endian platforms yet - todo"
#endif

/**
 * \brief Constructs a new Keccak sponge function.
 *
 * The capacity() will initially be set to 1536, which normally won't be
 * of much use to the caller.  The constructor should be followed by a
 * call to setCapacity() to select the capacity of interest.
 */
KeccakCore::KeccakCore()
	: _blockSize(8)
{
	memset(state.A, 0, sizeof(state.A));
	state.inputSize = 0;
	state.outputSize = 0;
}

/**
 * \brief Destroys this Keccak sponge function after clearing all
 * sensitive information.
 */
KeccakCore::~KeccakCore()
{
	clean(state);
}

/**
 * \brief Returns the capacity of the sponge function in bits.
 *
 * \sa setCapacity(), blockSize()
 */
size_t KeccakCore::capacity() const
{
	return 1600 - ((size_t)_blockSize) * 8;
}

/**
 * \brief Sets the capacity of the Keccak sponge function in bits.
 *
 * \param capacity The capacity of the Keccak sponge function in bits which
 * should be a multiple of 64 and between 64 and 1536.
 *
 * \note It is possible to create a sponge function with this constructor that
 * doesn't strictly conform with the capacity and hash size constraints
 * defined in the relevant standards.  It is the responsibility of callers
 * to only use standard parameter combinations.
 *
 * \sa capacity(), blockSize()
 */
void KeccakCore::setCapacity(size_t capacity)
{
	_blockSize = (1600 - capacity) / 8;
	reset();
}

/**
 * \fn size_t KeccakCore::blockSize() const
 * \brief Returns the input block size for the sponge function in bytes.
 *
 * The block size is (1600 - capacity()) / 8.
 *
 * \sa capacity()
 */

/**
 * \brief Resets the Keccak sponge function ready for a new session.
 *
 * \sa update(), extract()
 */
void KeccakCore::reset()
{
	memset(state.A, 0, sizeof(state.A));
	state.inputSize = 0;
	state.outputSize = 0;
}

/**
 * \brief Updates the Keccak sponge function with more input data.
 *
 * \param data The extra input data to incorporate.
 * \param size The size of the new data to incorporate.
 *
 * This function will invoke the sponge function whenever a full blockSize()
 * bytes of input data have been accumulated.  Call pad() after the last
 * block to finalize the input before calling extract().
 *
 * \sa pad(), extract(), reset()
 */
void KeccakCore::update(const void *data, size_t size)
{
	// Stop generating output while we incorporate the new data.
	state.outputSize = 0;

	// Break the input up into chunks and process each in turn.
	const uint8_t *d = (const uint8_t *)data;
	while (size > 0) {
		uint8_t len = _blockSize - state.inputSize;
		if (len > size) {
			len = size;
		}
		uint8_t *Abytes = ((uint8_t *)state.A) + state.inputSize;
		for (uint8_t posn = 0; posn < len; ++posn) {
			Abytes[posn] ^= d[posn];
		}
		state.inputSize += len;
		size -= len;
		d += len;
		if (state.inputSize == _blockSize) {
			keccakp();
			state.inputSize = 0;
		}
	}
}

/**
 * \brief Pads the last block of input data to blockSize().
 *
 * \param tag The tag byte to add to the padding to identify SHA3 (0x06),
 * SHAKE (0x1F), or the plain pre-standardized version of Keccak (0x01).
 *
 * The sponge function will be invoked to process the completed padding block.
 *
 * \sa update(), extract()
 */
void KeccakCore::pad(uint8_t tag)
{
	// Padding for SHA3-NNN variants according to FIPS 202 appends "01",
	// then another "1", then many zero bits, followed by a final "1".
	// SHAKE appends "1111" first instead of "01".  Note that SHA-3 numbers
	// bits from the least significant, so appending "01" is equivalent
	// to 0x02 for byte-aligned data, not 0x40.
	uint8_t size = state.inputSize;
	uint64_t *Awords = &(state.A[0][0]);
	Awords[size / 8] ^= (((uint64_t)tag) << ((size % 8) * 8));
	Awords[(_blockSize - 1) / 8] ^= 0x8000000000000000ULL;
	keccakp();
	state.inputSize = 0;
	state.outputSize = 0;
}

/**
 * \brief Extracts data from the Keccak sponge function.
 *
 * \param data The data buffer to fill with extracted data.
 * \param size The number number of bytes of extracted data that are required.
 *
 * If more than blockSize() bytes are required, the sponge function will
 * be invoked to generate additional data.
 *
 * \sa update(), reset(), encrypt()
 */
void KeccakCore::extract(void *data, size_t size)
{
	// Stop accepting input while we are generating output.
	state.inputSize = 0;

	// Copy the output data into the caller's return buffer.
	uint8_t *d = (uint8_t *)data;
	uint8_t tempSize;
	while (size > 0) {
		// Generate another output block if the current one has been exhausted.
		if (state.outputSize >= _blockSize) {
			keccakp();
			state.outputSize = 0;
		}

		// How many bytes can we copy this time around?
		tempSize = _blockSize - state.outputSize;
		if (tempSize > size) {
			tempSize = size;
		}

		// Copy the partial output data into the caller's return buffer.
		memcpy(d, ((uint8_t *)(state.A)) + state.outputSize, tempSize);
		state.outputSize += tempSize;
		size -= tempSize;
		d += tempSize;
	}
}

/**
 * \brief Extracts data from the Keccak sponge function and uses it to
 * encrypt a buffer.
 *
 * \param output The output buffer to write to, which may be the same
 * buffer as \a input.  The \a output buffer must have at least as many
 * bytes as the \a input buffer.
 * \param input The input buffer to read from.
 * \param size The number of bytes to encrypt.
 *
 * This function extracts data from the sponge function and then XOR's
 * it with \a input to generate the \a output.
 *
 * If more than blockSize() bytes are required, the sponge function will
 * be invoked to generate additional data.
 *
 * \sa update(), reset(), extract()
 */
void KeccakCore::encrypt(void *output, const void *input, size_t size)
{
	// Stop accepting input while we are generating output.
	state.inputSize = 0;

	// Copy the output data into the caller's return buffer.
	uint8_t *out = (uint8_t *)output;
	const uint8_t *in = (const uint8_t *)input;
	uint8_t tempSize;
	while (size > 0) {
		// Generate another output block if the current one has been exhausted.
		if (state.outputSize >= _blockSize) {
			keccakp();
			state.outputSize = 0;
		}

		// How many bytes can we extract this time around?
		tempSize = _blockSize - state.outputSize;
		if (tempSize > size) {
			tempSize = size;
		}

		// XOR the partial output data into the caller's return buffer.
		const uint8_t *d = ((const uint8_t *)(state.A)) + state.outputSize;
		for (uint8_t index = 0; index < tempSize; ++index) {
			out[index] = in[index] ^ d[index];
		}
		state.outputSize += tempSize;
		size -= tempSize;
		out += tempSize;
		in += tempSize;
	}
}

/**
 * \brief Clears all sensitive data from this object.
 */
void KeccakCore::clear()
{
	clean(state);
}

/**
 * \brief Sets a HMAC key for a Keccak-based hash algorithm.
 *
 * \param key Points to the HMAC key for the hashing process.
 * \param len Length of the HMAC \a key in bytes.
 * \param pad Inner (0x36) or outer (0x5C) padding value to XOR with
 * the formatted HMAC key.
 * \param hashSize The size of the output from the hash algorithm.
 *
 * This function is intended to help classes implement Hash::resetHMAC() and
 * Hash::finalizeHMAC() by directly formatting the HMAC key into the
 * internal block buffer and resetting the hash.
 */
void KeccakCore::setHMACKey(const void *key, size_t len, uint8_t pad, size_t hashSize)
{
	uint8_t *Abytes = (uint8_t *)state.A;
	size_t size = blockSize();
	reset();
	if (len <= size) {
		// Because the state has just been reset, state.A is set to
		// all-zeroes.  We can copy the key directly into the state
		// and then XOR the block with the pad value.
		memcpy(Abytes, key, len);
	} else {
		// The key is larger than the block size.  Hash it down.
		// Afterwards, state.A will contain the first block of data
		// to be extracted.  We truncate it to the first "hashSize"
		// bytes and XOR with the padding.
		update(key, len);
		this->pad(0x06);
		memset(Abytes + hashSize, pad, size - hashSize);
		memset(Abytes + size, 0, sizeof(state.A) - size);
		size = hashSize;
	}
	while (size > 0) {
		*Abytes++ ^= pad;
		--size;
	}
	keccakp();
}

/**
 * \brief Transform the state with the KECCAK-p sponge function with b = 1600.
 */
void KeccakCore::keccakp()
{
	uint64_t B[5][5];
#if defined(__AVR__)
	// This assembly code was generated by the "genkeccak.c" program.
	// Do not modify this code directly.  Instead modify "genkeccak.c"
	// and then re-generate the code here.
	for (uint8_t round = 0; round < 24; ++round) {
		__asm__ __volatile__ (
		    "push r29\n"
		    "push r28\n"
		    "mov r28,r26\n"
		    "mov r29,r27\n"

		    // Step mapping theta.  Compute C.
		    "ldi r20,5\n"
		    "100:\n"
		    "ld r8,Z\n"
		    "ldd r9,Z+1\n"
		    "ldd r10,Z+2\n"
		    "ldd r11,Z+3\n"
		    "ldd r12,Z+4\n"
		    "ldd r13,Z+5\n"
		    "ldd r14,Z+6\n"
		    "ldd r15,Z+7\n"
		    "ldi r19,4\n"
		    "101:\n"
		    "adiw r30,40\n"
		    "ld __tmp_reg__,Z\n"
		    "eor r8,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+1\n"
		    "eor r9,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+2\n"
		    "eor r10,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+3\n"
		    "eor r11,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+4\n"
		    "eor r12,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+5\n"
		    "eor r13,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+6\n"
		    "eor r14,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+7\n"
		    "eor r15,__tmp_reg__\n"
		    "dec r19\n"
		    "brne 101b\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "subi r30,152\n"
		    "sbc r31,__zero_reg__\n"
		    "dec r20\n"
		    "brne 100b\n"
		    "sbiw r30,40\n"
		    "sbiw r26,40\n"

		    // Step mapping theta.  Compute D and XOR with A.
		    "ldd r8,Y+8\n"
		    "ldd r9,Y+9\n"
		    "ldd r10,Y+10\n"
		    "ldd r11,Y+11\n"
		    "ldd r12,Y+12\n"
		    "ldd r13,Y+13\n"
		    "ldd r14,Y+14\n"
		    "ldd r15,Y+15\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "ldd __tmp_reg__,Y+32\n"
		    "eor r8,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+33\n"
		    "eor r9,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+34\n"
		    "eor r10,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+35\n"
		    "eor r11,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+36\n"
		    "eor r12,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+37\n"
		    "eor r13,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+38\n"
		    "eor r14,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+39\n"
		    "eor r15,__tmp_reg__\n"
		    "ldi r19,5\n"
		    "103:\n"
		    "ld __tmp_reg__,Z\n"
		    "eor __tmp_reg__,r8\n"
		    "st Z,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+1\n"
		    "eor __tmp_reg__,r9\n"
		    "std Z+1,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+2\n"
		    "eor __tmp_reg__,r10\n"
		    "std Z+2,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+3\n"
		    "eor __tmp_reg__,r11\n"
		    "std Z+3,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+4\n"
		    "eor __tmp_reg__,r12\n"
		    "std Z+4,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+5\n"
		    "eor __tmp_reg__,r13\n"
		    "std Z+5,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+6\n"
		    "eor __tmp_reg__,r14\n"
		    "std Z+6,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+7\n"
		    "eor __tmp_reg__,r15\n"
		    "std Z+7,__tmp_reg__\n"
		    "adiw r30,40\n"
		    "dec r19\n"
		    "brne 103b\n"
		    "subi r30,192\n"
		    "sbc r31,__zero_reg__\n"
		    "ldd r8,Y+16\n"
		    "ldd r9,Y+17\n"
		    "ldd r10,Y+18\n"
		    "ldd r11,Y+19\n"
		    "ldd r12,Y+20\n"
		    "ldd r13,Y+21\n"
		    "ldd r14,Y+22\n"
		    "ldd r15,Y+23\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "ld __tmp_reg__,Y\n"
		    "eor r8,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+1\n"
		    "eor r9,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+2\n"
		    "eor r10,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+3\n"
		    "eor r11,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+4\n"
		    "eor r12,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+5\n"
		    "eor r13,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+6\n"
		    "eor r14,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+7\n"
		    "eor r15,__tmp_reg__\n"
		    "ldi r19,5\n"
		    "104:\n"
		    "ld __tmp_reg__,Z\n"
		    "eor __tmp_reg__,r8\n"
		    "st Z,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+1\n"
		    "eor __tmp_reg__,r9\n"
		    "std Z+1,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+2\n"
		    "eor __tmp_reg__,r10\n"
		    "std Z+2,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+3\n"
		    "eor __tmp_reg__,r11\n"
		    "std Z+3,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+4\n"
		    "eor __tmp_reg__,r12\n"
		    "std Z+4,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+5\n"
		    "eor __tmp_reg__,r13\n"
		    "std Z+5,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+6\n"
		    "eor __tmp_reg__,r14\n"
		    "std Z+6,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+7\n"
		    "eor __tmp_reg__,r15\n"
		    "std Z+7,__tmp_reg__\n"
		    "adiw r30,40\n"
		    "dec r19\n"
		    "brne 104b\n"
		    "subi r30,192\n"
		    "sbc r31,__zero_reg__\n"
		    "ldd r8,Y+24\n"
		    "ldd r9,Y+25\n"
		    "ldd r10,Y+26\n"
		    "ldd r11,Y+27\n"
		    "ldd r12,Y+28\n"
		    "ldd r13,Y+29\n"
		    "ldd r14,Y+30\n"
		    "ldd r15,Y+31\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "ldd __tmp_reg__,Y+8\n"
		    "eor r8,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+9\n"
		    "eor r9,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+10\n"
		    "eor r10,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+11\n"
		    "eor r11,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+12\n"
		    "eor r12,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+13\n"
		    "eor r13,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+14\n"
		    "eor r14,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+15\n"
		    "eor r15,__tmp_reg__\n"
		    "ldi r19,5\n"
		    "105:\n"
		    "ld __tmp_reg__,Z\n"
		    "eor __tmp_reg__,r8\n"
		    "st Z,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+1\n"
		    "eor __tmp_reg__,r9\n"
		    "std Z+1,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+2\n"
		    "eor __tmp_reg__,r10\n"
		    "std Z+2,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+3\n"
		    "eor __tmp_reg__,r11\n"
		    "std Z+3,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+4\n"
		    "eor __tmp_reg__,r12\n"
		    "std Z+4,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+5\n"
		    "eor __tmp_reg__,r13\n"
		    "std Z+5,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+6\n"
		    "eor __tmp_reg__,r14\n"
		    "std Z+6,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+7\n"
		    "eor __tmp_reg__,r15\n"
		    "std Z+7,__tmp_reg__\n"
		    "adiw r30,40\n"
		    "dec r19\n"
		    "brne 105b\n"
		    "subi r30,192\n"
		    "sbc r31,__zero_reg__\n"
		    "ldd r8,Y+32\n"
		    "ldd r9,Y+33\n"
		    "ldd r10,Y+34\n"
		    "ldd r11,Y+35\n"
		    "ldd r12,Y+36\n"
		    "ldd r13,Y+37\n"
		    "ldd r14,Y+38\n"
		    "ldd r15,Y+39\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "ldd __tmp_reg__,Y+16\n"
		    "eor r8,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+17\n"
		    "eor r9,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+18\n"
		    "eor r10,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+19\n"
		    "eor r11,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+20\n"
		    "eor r12,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+21\n"
		    "eor r13,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+22\n"
		    "eor r14,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+23\n"
		    "eor r15,__tmp_reg__\n"
		    "ldi r19,5\n"
		    "106:\n"
		    "ld __tmp_reg__,Z\n"
		    "eor __tmp_reg__,r8\n"
		    "st Z,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+1\n"
		    "eor __tmp_reg__,r9\n"
		    "std Z+1,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+2\n"
		    "eor __tmp_reg__,r10\n"
		    "std Z+2,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+3\n"
		    "eor __tmp_reg__,r11\n"
		    "std Z+3,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+4\n"
		    "eor __tmp_reg__,r12\n"
		    "std Z+4,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+5\n"
		    "eor __tmp_reg__,r13\n"
		    "std Z+5,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+6\n"
		    "eor __tmp_reg__,r14\n"
		    "std Z+6,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+7\n"
		    "eor __tmp_reg__,r15\n"
		    "std Z+7,__tmp_reg__\n"
		    "adiw r30,40\n"
		    "dec r19\n"
		    "brne 106b\n"
		    "subi r30,192\n"
		    "sbc r31,__zero_reg__\n"
		    "ld r8,Y\n"
		    "ldd r9,Y+1\n"
		    "ldd r10,Y+2\n"
		    "ldd r11,Y+3\n"
		    "ldd r12,Y+4\n"
		    "ldd r13,Y+5\n"
		    "ldd r14,Y+6\n"
		    "ldd r15,Y+7\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "ldd __tmp_reg__,Y+24\n"
		    "eor r8,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+25\n"
		    "eor r9,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+26\n"
		    "eor r10,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+27\n"
		    "eor r11,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+28\n"
		    "eor r12,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+29\n"
		    "eor r13,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+30\n"
		    "eor r14,__tmp_reg__\n"
		    "ldd __tmp_reg__,Y+31\n"
		    "eor r15,__tmp_reg__\n"
		    "ldi r19,5\n"
		    "107:\n"
		    "ld __tmp_reg__,Z\n"
		    "eor __tmp_reg__,r8\n"
		    "st Z,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+1\n"
		    "eor __tmp_reg__,r9\n"
		    "std Z+1,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+2\n"
		    "eor __tmp_reg__,r10\n"
		    "std Z+2,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+3\n"
		    "eor __tmp_reg__,r11\n"
		    "std Z+3,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+4\n"
		    "eor __tmp_reg__,r12\n"
		    "std Z+4,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+5\n"
		    "eor __tmp_reg__,r13\n"
		    "std Z+5,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+6\n"
		    "eor __tmp_reg__,r14\n"
		    "std Z+6,__tmp_reg__\n"
		    "ldd __tmp_reg__,Z+7\n"
		    "eor __tmp_reg__,r15\n"
		    "std Z+7,__tmp_reg__\n"
		    "adiw r30,40\n"
		    "dec r19\n"
		    "brne 107b\n"
		    "subi r30,232\n"
		    "sbc r31,__zero_reg__\n"

		    // Step mappings rho and pi combined into one step.

		    // B[0][0] = A[0][0]
		    "ld r8,Z\n"
		    "ldd r9,Z+1\n"
		    "ldd r10,Z+2\n"
		    "ldd r11,Z+3\n"
		    "ldd r12,Z+4\n"
		    "ldd r13,Z+5\n"
		    "ldd r14,Z+6\n"
		    "ldd r15,Z+7\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"

		    // B[1][0] = leftRotate28_64(A[0][3])
		    "adiw r26,32\n"
		    "ldd r8,Z+24\n"
		    "ldd r9,Z+25\n"
		    "ldd r10,Z+26\n"
		    "ldd r11,Z+27\n"
		    "ldd r12,Z+28\n"
		    "ldd r13,Z+29\n"
		    "ldd r14,Z+30\n"
		    "ldd r15,Z+31\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"

		    // B[2][0] = leftRotate1_64(A[0][1])
		    "adiw r26,32\n"
		    "ldd r8,Z+8\n"
		    "ldd r9,Z+9\n"
		    "ldd r10,Z+10\n"
		    "ldd r11,Z+11\n"
		    "ldd r12,Z+12\n"
		    "ldd r13,Z+13\n"
		    "ldd r14,Z+14\n"
		    "ldd r15,Z+15\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"

		    // B[3][0] = leftRotate27_64(A[0][4])
		    "adiw r26,32\n"
		    "ldd r8,Z+32\n"
		    "ldd r9,Z+33\n"
		    "ldd r10,Z+34\n"
		    "ldd r11,Z+35\n"
		    "ldd r12,Z+36\n"
		    "ldd r13,Z+37\n"
		    "ldd r14,Z+38\n"
		    "ldd r15,Z+39\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"

		    // B[4][0] = leftRotate62_64(A[0][2])
		    "adiw r26,32\n"
		    "ldd r8,Z+16\n"
		    "ldd r9,Z+17\n"
		    "ldd r10,Z+18\n"
		    "ldd r11,Z+19\n"
		    "ldd r12,Z+20\n"
		    "ldd r13,Z+21\n"
		    "ldd r14,Z+22\n"
		    "ldd r15,Z+23\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"

		    // B[0][1] = leftRotate44_64(A[1][1])
		    "subi r26,160\n"
		    "sbc r27,__zero_reg__\n"
		    "adiw r30,40\n"
		    "ldd r8,Z+8\n"
		    "ldd r9,Z+9\n"
		    "ldd r10,Z+10\n"
		    "ldd r11,Z+11\n"
		    "ldd r12,Z+12\n"
		    "ldd r13,Z+13\n"
		    "ldd r14,Z+14\n"
		    "ldd r15,Z+15\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"

		    // B[1][1] = leftRotate20_64(A[1][4])
		    "adiw r26,32\n"
		    "ldd r8,Z+32\n"
		    "ldd r9,Z+33\n"
		    "ldd r10,Z+34\n"
		    "ldd r11,Z+35\n"
		    "ldd r12,Z+36\n"
		    "ldd r13,Z+37\n"
		    "ldd r14,Z+38\n"
		    "ldd r15,Z+39\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"

		    // B[2][1] = leftRotate6_64(A[1][2])
		    "adiw r26,32\n"
		    "ldd r8,Z+16\n"
		    "ldd r9,Z+17\n"
		    "ldd r10,Z+18\n"
		    "ldd r11,Z+19\n"
		    "ldd r12,Z+20\n"
		    "ldd r13,Z+21\n"
		    "ldd r14,Z+22\n"
		    "ldd r15,Z+23\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"

		    // B[3][1] = leftRotate36_64(A[1][0])
		    "adiw r26,32\n"
		    "ld r8,Z\n"
		    "ldd r9,Z+1\n"
		    "ldd r10,Z+2\n"
		    "ldd r11,Z+3\n"
		    "ldd r12,Z+4\n"
		    "ldd r13,Z+5\n"
		    "ldd r14,Z+6\n"
		    "ldd r15,Z+7\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"

		    // B[4][1] = leftRotate55_64(A[1][3])
		    "adiw r26,32\n"
		    "ldd r8,Z+24\n"
		    "ldd r9,Z+25\n"
		    "ldd r10,Z+26\n"
		    "ldd r11,Z+27\n"
		    "ldd r12,Z+28\n"
		    "ldd r13,Z+29\n"
		    "ldd r14,Z+30\n"
		    "ldd r15,Z+31\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"

		    // B[0][2] = leftRotate43_64(A[2][2])
		    "subi r26,160\n"
		    "sbc r27,__zero_reg__\n"
		    "adiw r30,40\n"
		    "ldd r8,Z+16\n"
		    "ldd r9,Z+17\n"
		    "ldd r10,Z+18\n"
		    "ldd r11,Z+19\n"
		    "ldd r12,Z+20\n"
		    "ldd r13,Z+21\n"
		    "ldd r14,Z+22\n"
		    "ldd r15,Z+23\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"

		    // B[1][2] = leftRotate3_64(A[2][0])
		    "adiw r26,32\n"
		    "ld r8,Z\n"
		    "ldd r9,Z+1\n"
		    "ldd r10,Z+2\n"
		    "ldd r11,Z+3\n"
		    "ldd r12,Z+4\n"
		    "ldd r13,Z+5\n"
		    "ldd r14,Z+6\n"
		    "ldd r15,Z+7\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"

		    // B[2][2] = leftRotate25_64(A[2][3])
		    "adiw r26,32\n"
		    "ldd r8,Z+24\n"
		    "ldd r9,Z+25\n"
		    "ldd r10,Z+26\n"
		    "ldd r11,Z+27\n"
		    "ldd r12,Z+28\n"
		    "ldd r13,Z+29\n"
		    "ldd r14,Z+30\n"
		    "ldd r15,Z+31\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"

		    // B[3][2] = leftRotate10_64(A[2][1])
		    "adiw r26,32\n"
		    "ldd r8,Z+8\n"
		    "ldd r9,Z+9\n"
		    "ldd r10,Z+10\n"
		    "ldd r11,Z+11\n"
		    "ldd r12,Z+12\n"
		    "ldd r13,Z+13\n"
		    "ldd r14,Z+14\n"
		    "ldd r15,Z+15\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"

		    // B[4][2] = leftRotate39_64(A[2][4])
		    "adiw r26,32\n"
		    "ldd r8,Z+32\n"
		    "ldd r9,Z+33\n"
		    "ldd r10,Z+34\n"
		    "ldd r11,Z+35\n"
		    "ldd r12,Z+36\n"
		    "ldd r13,Z+37\n"
		    "ldd r14,Z+38\n"
		    "ldd r15,Z+39\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"

		    // B[0][3] = leftRotate21_64(A[3][3])
		    "subi r26,160\n"
		    "sbc r27,__zero_reg__\n"
		    "adiw r30,40\n"
		    "ldd r8,Z+24\n"
		    "ldd r9,Z+25\n"
		    "ldd r10,Z+26\n"
		    "ldd r11,Z+27\n"
		    "ldd r12,Z+28\n"
		    "ldd r13,Z+29\n"
		    "ldd r14,Z+30\n"
		    "ldd r15,Z+31\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"

		    // B[1][3] = leftRotate45_64(A[3][1])
		    "adiw r26,32\n"
		    "ldd r8,Z+8\n"
		    "ldd r9,Z+9\n"
		    "ldd r10,Z+10\n"
		    "ldd r11,Z+11\n"
		    "ldd r12,Z+12\n"
		    "ldd r13,Z+13\n"
		    "ldd r14,Z+14\n"
		    "ldd r15,Z+15\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"

		    // B[2][3] = leftRotate8_64(A[3][4])
		    "adiw r26,32\n"
		    "ldd r8,Z+32\n"
		    "ldd r9,Z+33\n"
		    "ldd r10,Z+34\n"
		    "ldd r11,Z+35\n"
		    "ldd r12,Z+36\n"
		    "ldd r13,Z+37\n"
		    "ldd r14,Z+38\n"
		    "ldd r15,Z+39\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"

		    // B[3][3] = leftRotate15_64(A[3][2])
		    "adiw r26,32\n"
		    "ldd r8,Z+16\n"
		    "ldd r9,Z+17\n"
		    "ldd r10,Z+18\n"
		    "ldd r11,Z+19\n"
		    "ldd r12,Z+20\n"
		    "ldd r13,Z+21\n"
		    "ldd r14,Z+22\n"
		    "ldd r15,Z+23\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"

		    // B[4][3] = leftRotate41_64(A[3][0])
		    "adiw r26,32\n"
		    "ld r8,Z\n"
		    "ldd r9,Z+1\n"
		    "ldd r10,Z+2\n"
		    "ldd r11,Z+3\n"
		    "ldd r12,Z+4\n"
		    "ldd r13,Z+5\n"
		    "ldd r14,Z+6\n"
		    "ldd r15,Z+7\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"

		    // B[0][4] = leftRotate14_64(A[4][4])
		    "subi r26,160\n"
		    "sbc r27,__zero_reg__\n"
		    "adiw r30,40\n"
		    "ldd r8,Z+32\n"
		    "ldd r9,Z+33\n"
		    "ldd r10,Z+34\n"
		    "ldd r11,Z+35\n"
		    "ldd r12,Z+36\n"
		    "ldd r13,Z+37\n"
		    "ldd r14,Z+38\n"
		    "ldd r15,Z+39\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"

		    // B[1][4] = leftRotate61_64(A[4][2])
		    "adiw r26,32\n"
		    "ldd r8,Z+16\n"
		    "ldd r9,Z+17\n"
		    "ldd r10,Z+18\n"
		    "ldd r11,Z+19\n"
		    "ldd r12,Z+20\n"
		    "ldd r13,Z+21\n"
		    "ldd r14,Z+22\n"
		    "ldd r15,Z+23\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "bst r8,0\n"
		    "ror r15\n"
		    "ror r14\n"
		    "ror r13\n"
		    "ror r12\n"
		    "ror r11\n"
		    "ror r10\n"
		    "ror r9\n"
		    "ror r8\n"
		    "bld r15,7\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"

		    // B[2][4] = leftRotate18_64(A[4][0])
		    "adiw r26,32\n"
		    "ld r8,Z\n"
		    "ldd r9,Z+1\n"
		    "ldd r10,Z+2\n"
		    "ldd r11,Z+3\n"
		    "ldd r12,Z+4\n"
		    "ldd r13,Z+5\n"
		    "ldd r14,Z+6\n"
		    "ldd r15,Z+7\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"

		    // B[3][4] = leftRotate56_64(A[4][3])
		    "adiw r26,32\n"
		    "ldd r8,Z+24\n"
		    "ldd r9,Z+25\n"
		    "ldd r10,Z+26\n"
		    "ldd r11,Z+27\n"
		    "ldd r12,Z+28\n"
		    "ldd r13,Z+29\n"
		    "ldd r14,Z+30\n"
		    "ldd r15,Z+31\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "st X+,r8\n"

		    // B[4][4] = leftRotate2_64(A[4][1])
		    "adiw r26,32\n"
		    "ldd r8,Z+8\n"
		    "ldd r9,Z+9\n"
		    "ldd r10,Z+10\n"
		    "ldd r11,Z+11\n"
		    "ldd r12,Z+12\n"
		    "ldd r13,Z+13\n"
		    "ldd r14,Z+14\n"
		    "ldd r15,Z+15\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "lsl r8\n"
		    "rol r9\n"
		    "rol r10\n"
		    "rol r11\n"
		    "rol r12\n"
		    "rol r13\n"
		    "rol r14\n"
		    "rol r15\n"
		    "adc r8, __zero_reg__\n"
		    "st X+,r8\n"
		    "st X+,r9\n"
		    "st X+,r10\n"
		    "st X+,r11\n"
		    "st X+,r12\n"
		    "st X+,r13\n"
		    "st X+,r14\n"
		    "st X+,r15\n"
		    "subi r26,200\n"
		    "sbc r27,__zero_reg__\n"
		    "subi r30,160\n"
		    "sbc r31,__zero_reg__\n"

		    // Step mapping chi.
		    "ldi r20,5\n"
		    "50:\n"
		    "ld r8,Y\n"
		    "ldd r9,Y+8\n"
		    "ldd r10,Y+16\n"
		    "ldd r11,Y+24\n"
		    "ldd r12,Y+32\n"
		    "mov r13,r9\n"
		    "com r13\n"
		    "and r13,r10\n"
		    "eor r13,r8\n"
		    "mov r14,r10\n"
		    "com r14\n"
		    "and r14,r11\n"
		    "eor r14,r9\n"
		    "mov r15,r11\n"
		    "com r15\n"
		    "and r15,r12\n"
		    "eor r15,r10\n"
		    "mov r17,r12\n"
		    "com r17\n"
		    "and r17,r8\n"
		    "eor r17,r11\n"
		    "mov r16,r8\n"
		    "com r16\n"
		    "and r16,r9\n"
		    "eor r16,r12\n"
		    "st Z,r13\n"
		    "std Z+8,r14\n"
		    "std Z+16,r15\n"
		    "std Z+24,r17\n"
		    "std Z+32,r16\n"
		    "ldd r8,Y+1\n"
		    "ldd r9,Y+9\n"
		    "ldd r10,Y+17\n"
		    "ldd r11,Y+25\n"
		    "ldd r12,Y+33\n"
		    "mov r13,r9\n"
		    "com r13\n"
		    "and r13,r10\n"
		    "eor r13,r8\n"
		    "mov r14,r10\n"
		    "com r14\n"
		    "and r14,r11\n"
		    "eor r14,r9\n"
		    "mov r15,r11\n"
		    "com r15\n"
		    "and r15,r12\n"
		    "eor r15,r10\n"
		    "mov r17,r12\n"
		    "com r17\n"
		    "and r17,r8\n"
		    "eor r17,r11\n"
		    "mov r16,r8\n"
		    "com r16\n"
		    "and r16,r9\n"
		    "eor r16,r12\n"
		    "std Z+1,r13\n"
		    "std Z+9,r14\n"
		    "std Z+17,r15\n"
		    "std Z+25,r17\n"
		    "std Z+33,r16\n"
		    "ldd r8,Y+2\n"
		    "ldd r9,Y+10\n"
		    "ldd r10,Y+18\n"
		    "ldd r11,Y+26\n"
		    "ldd r12,Y+34\n"
		    "mov r13,r9\n"
		    "com r13\n"
		    "and r13,r10\n"
		    "eor r13,r8\n"
		    "mov r14,r10\n"
		    "com r14\n"
		    "and r14,r11\n"
		    "eor r14,r9\n"
		    "mov r15,r11\n"
		    "com r15\n"
		    "and r15,r12\n"
		    "eor r15,r10\n"
		    "mov r17,r12\n"
		    "com r17\n"
		    "and r17,r8\n"
		    "eor r17,r11\n"
		    "mov r16,r8\n"
		    "com r16\n"
		    "and r16,r9\n"
		    "eor r16,r12\n"
		    "std Z+2,r13\n"
		    "std Z+10,r14\n"
		    "std Z+18,r15\n"
		    "std Z+26,r17\n"
		    "std Z+34,r16\n"
		    "ldd r8,Y+3\n"
		    "ldd r9,Y+11\n"
		    "ldd r10,Y+19\n"
		    "ldd r11,Y+27\n"
		    "ldd r12,Y+35\n"
		    "mov r13,r9\n"
		    "com r13\n"
		    "and r13,r10\n"
		    "eor r13,r8\n"
		    "mov r14,r10\n"
		    "com r14\n"
		    "and r14,r11\n"
		    "eor r14,r9\n"
		    "mov r15,r11\n"
		    "com r15\n"
		    "and r15,r12\n"
		    "eor r15,r10\n"
		    "mov r17,r12\n"
		    "com r17\n"
		    "and r17,r8\n"
		    "eor r17,r11\n"
		    "mov r16,r8\n"
		    "com r16\n"
		    "and r16,r9\n"
		    "eor r16,r12\n"
		    "std Z+3,r13\n"
		    "std Z+11,r14\n"
		    "std Z+19,r15\n"
		    "std Z+27,r17\n"
		    "std Z+35,r16\n"
		    "ldd r8,Y+4\n"
		    "ldd r9,Y+12\n"
		    "ldd r10,Y+20\n"
		    "ldd r11,Y+28\n"
		    "ldd r12,Y+36\n"
		    "mov r13,r9\n"
		    "com r13\n"
		    "and r13,r10\n"
		    "eor r13,r8\n"
		    "mov r14,r10\n"
		    "com r14\n"
		    "and r14,r11\n"
		    "eor r14,r9\n"
		    "mov r15,r11\n"
		    "com r15\n"
		    "and r15,r12\n"
		    "eor r15,r10\n"
		    "mov r17,r12\n"
		    "com r17\n"
		    "and r17,r8\n"
		    "eor r17,r11\n"
		    "mov r16,r8\n"
		    "com r16\n"
		    "and r16,r9\n"
		    "eor r16,r12\n"
		    "std Z+4,r13\n"
		    "std Z+12,r14\n"
		    "std Z+20,r15\n"
		    "std Z+28,r17\n"
		    "std Z+36,r16\n"
		    "ldd r8,Y+5\n"
		    "ldd r9,Y+13\n"
		    "ldd r10,Y+21\n"
		    "ldd r11,Y+29\n"
		    "ldd r12,Y+37\n"
		    "mov r13,r9\n"
		    "com r13\n"
		    "and r13,r10\n"
		    "eor r13,r8\n"
		    "mov r14,r10\n"
		    "com r14\n"
		    "and r14,r11\n"
		    "eor r14,r9\n"
		    "mov r15,r11\n"
		    "com r15\n"
		    "and r15,r12\n"
		    "eor r15,r10\n"
		    "mov r17,r12\n"
		    "com r17\n"
		    "and r17,r8\n"
		    "eor r17,r11\n"
		    "mov r16,r8\n"
		    "com r16\n"
		    "and r16,r9\n"
		    "eor r16,r12\n"
		    "std Z+5,r13\n"
		    "std Z+13,r14\n"
		    "std Z+21,r15\n"
		    "std Z+29,r17\n"
		    "std Z+37,r16\n"
		    "ldd r8,Y+6\n"
		    "ldd r9,Y+14\n"
		    "ldd r10,Y+22\n"
		    "ldd r11,Y+30\n"
		    "ldd r12,Y+38\n"
		    "mov r13,r9\n"
		    "com r13\n"
		    "and r13,r10\n"
		    "eor r13,r8\n"
		    "mov r14,r10\n"
		    "com r14\n"
		    "and r14,r11\n"
		    "eor r14,r9\n"
		    "mov r15,r11\n"
		    "com r15\n"
		    "and r15,r12\n"
		    "eor r15,r10\n"
		    "mov r17,r12\n"
		    "com r17\n"
		    "and r17,r8\n"
		    "eor r17,r11\n"
		    "mov r16,r8\n"
		    "com r16\n"
		    "and r16,r9\n"
		    "eor r16,r12\n"
		    "std Z+6,r13\n"
		    "std Z+14,r14\n"
		    "std Z+22,r15\n"
		    "std Z+30,r17\n"
		    "std Z+38,r16\n"
		    "ldd r8,Y+7\n"
		    "ldd r9,Y+15\n"
		    "ldd r10,Y+23\n"
		    "ldd r11,Y+31\n"
		    "ldd r12,Y+39\n"
		    "mov r13,r9\n"
		    "com r13\n"
		    "and r13,r10\n"
		    "eor r13,r8\n"
		    "mov r14,r10\n"
		    "com r14\n"
		    "and r14,r11\n"
		    "eor r14,r9\n"
		    "mov r15,r11\n"
		    "com r15\n"
		    "and r15,r12\n"
		    "eor r15,r10\n"
		    "mov r17,r12\n"
		    "com r17\n"
		    "and r17,r8\n"
		    "eor r17,r11\n"
		    "mov r16,r8\n"
		    "com r16\n"
		    "and r16,r9\n"
		    "eor r16,r12\n"
		    "std Z+7,r13\n"
		    "std Z+15,r14\n"
		    "std Z+23,r15\n"
		    "std Z+31,r17\n"
		    "std Z+39,r16\n"
		    "adiw r30,40\n"
		    "adiw r28,40\n"
		    "dec r20\n"
		    "breq 51f\n"
		    "rjmp 50b\n"
		    "51:\n"
		    "pop r28\n"
		    "pop r29\n"

		    // Done
		    : : "x"(B), "z"(state.A)
		    : "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
		    "r16", "r17", "r18", "r19", "r20", "r21", "memory"
		);
#else
	static const uint8_t addMod5Table[9] PROGMEM = {
		0, 1, 2, 3, 4, 0, 1, 2, 3
	};
#define addMod5(x, y) (pgm_read_byte(&(addMod5Table[(x) + (y)])))
	uint64_t D;
	uint8_t index, index2;
	for (uint8_t round = 0; round < 24; ++round) {
		// Step mapping theta.  The specification mentions two temporary
		// arrays of size 5 called C and D.  To save a bit of memory,
		// we use the first row of B to store C and compute D on the fly.
		for (index = 0; index < 5; ++index) {
			B[0][index] = state.A[0][index] ^ state.A[1][index] ^
			              state.A[2][index] ^ state.A[3][index] ^
			              state.A[4][index];
		}
		for (index = 0; index < 5; ++index) {
			D = B[0][addMod5(index, 4)] ^
			    leftRotate1_64(B[0][addMod5(index, 1)]);
			for (index2 = 0; index2 < 5; ++index2) {
				state.A[index2][index] ^= D;
			}
		}

		// Step mapping rho and pi combined into a single step.
		// Rotate all lanes by a specific offset and rearrange.
		B[0][0] = state.A[0][0];
		B[1][0] = leftRotate28_64(state.A[0][3]);
		B[2][0] = leftRotate1_64 (state.A[0][1]);
		B[3][0] = leftRotate27_64(state.A[0][4]);
		B[4][0] = leftRotate62_64(state.A[0][2]);
		B[0][1] = leftRotate44_64(state.A[1][1]);
		B[1][1] = leftRotate20_64(state.A[1][4]);
		B[2][1] = leftRotate6_64 (state.A[1][2]);
		B[3][1] = leftRotate36_64(state.A[1][0]);
		B[4][1] = leftRotate55_64(state.A[1][3]);
		B[0][2] = leftRotate43_64(state.A[2][2]);
		B[1][2] = leftRotate3_64 (state.A[2][0]);
		B[2][2] = leftRotate25_64(state.A[2][3]);
		B[3][2] = leftRotate10_64(state.A[2][1]);
		B[4][2] = leftRotate39_64(state.A[2][4]);
		B[0][3] = leftRotate21_64(state.A[3][3]);
		B[1][3] = leftRotate45_64(state.A[3][1]);
		B[2][3] = leftRotate8_64 (state.A[3][4]);
		B[3][3] = leftRotate15_64(state.A[3][2]);
		B[4][3] = leftRotate41_64(state.A[3][0]);
		B[0][4] = leftRotate14_64(state.A[4][4]);
		B[1][4] = leftRotate61_64(state.A[4][2]);
		B[2][4] = leftRotate18_64(state.A[4][0]);
		B[3][4] = leftRotate56_64(state.A[4][3]);
		B[4][4] = leftRotate2_64 (state.A[4][1]);

		// Step mapping chi.  Combine each lane with two other lanes in its row.
		for (index = 0; index < 5; ++index) {
			for (index2 = 0; index2 < 5; ++index2) {
				state.A[index2][index] =
				    B[index2][index] ^
				    ((~B[index2][addMod5(index, 1)]) &
				     B[index2][addMod5(index, 2)]);
			}
		}
#endif

		// Step mapping iota.  XOR A[0][0] with the round constant.
		static uint64_t const RC[24] PROGMEM = {
			0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808AULL,
			0x8000000080008000ULL, 0x000000000000808BULL, 0x0000000080000001ULL,
			0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008AULL,
			0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000AULL,
			0x000000008000808BULL, 0x800000000000008BULL, 0x8000000000008089ULL,
			0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
			0x000000000000800AULL, 0x800000008000000AULL, 0x8000000080008081ULL,
			0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
		};
		state.A[0][0] ^= pgm_read_qword(RC + round);
	}
}
