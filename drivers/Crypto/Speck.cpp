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

#include "Speck.h"
#include "Crypto.h"
#include "utility/RotateUtil.h"
#include "utility/EndianUtil.h"
#include <string.h>

/**
 * \class Speck Speck.h <Speck.h>
 * \brief Speck block cipher with a 128-bit block size.
 *
 * Speck is a family of lightweight block ciphers designed by the
 * National Security Agency (NSA).  The ciphers are highly optimized
 * for software implementation on microcontrollers.
 *
 * This class implements the Speck family that uses 128-bit block sizes
 * with 128-bit, 192-bit, or 256-bit key sizes.  Other Speck families support
 * smaller block sizes of 32, 48, 64, or 96 bits but such block sizes are
 * too small for use in modern cryptosystems.
 *
 * \note Current crytoanalysis (up until 2015) has not revealed any obvious
 * weaknesses in the full-round version of Speck.  But if you are wary of
 * ciphers designed by the NSA, then use ChaCha or AES instead.
 *
 * The SpeckTiny and SpeckSmall classes provide alternative implementations
 * that have reduced RAM and flash size requirements at the cost of some
 * features and performance.
 *
 * References: https://en.wikipedia.org/wiki/Speck_%28cipher%29,
 * http://eprint.iacr.org/2013/404
 *
 * \sa SpeckTiny, SpeckSmall
 */

// The "avr-gcc" compiler doesn't do a very good job of compiling
// code involving 64-bit values.  So we have to use inline assembly.
// It also helps to break the state up into 32-bit quantities
// because "asm" supports register names like %A0, %B0, %C0, %D0
// for the bytes in a 32-bit quantity, but it does not support
// %E0, %F0, %G0, %H0 for the high bytes of a 64-bit quantity.
#if defined(__AVR__)
#define USE_AVR_INLINE_ASM 1
#endif

/**
 * \brief Constructs a Speck block cipher with no initial key.
 *
 * This constructor must be followed by a call to setKey() before the
 * block cipher can be used for encryption or decryption.
 */
Speck::Speck()
	: rounds(32)
{
}

Speck::~Speck()
{
	clean(k);
}

size_t Speck::blockSize() const
{
	return 16;
}

size_t Speck::keySize() const
{
	// Also supports 128-bit and 192-bit, but we only report 256-bit.
	return 32;
}

// Pack/unpack byte-aligned big-endian 64-bit quantities.
#define pack64(data, value) \
	do { \
		uint64_t v = htobe64((value)); \
		memcpy((data), &v, sizeof(uint64_t)); \
	} while (0)
#define unpack64(value, data) \
	do { \
		memcpy(&(value), (data), sizeof(uint64_t)); \
		(value) = be64toh((value)); \
	} while (0)

bool Speck::setKey(const uint8_t *key, size_t len)
{
#if USE_AVR_INLINE_ASM
	uint64_t l[4];
	uint8_t m, mb;
	if (len == 32) {
		m = 4;
		mb = 3 * 8;
	} else if (len == 24) {
		m = 3;
		mb = 2 * 8;
	} else if (len == 16) {
		m = 2;
		mb = 8;
	} else {
		return false;
	}
	rounds = 30 + m;

	// Copy the first (m - 1) * 8 bytes of the key into the "l" array
	// in reverse order to convert big endian into little-endian.
	__asm__ __volatile__ (
	    "1:\n"
	    "ld __tmp_reg__,-Z\n"
	    "st X+,__tmp_reg__\n"
	    "dec %2\n"
	    "brne 1b\n"
	    : : "x"(l), "z"(key + len - 8), "r"(mb)
	);

	// Copy the final 8 bytes of the key into k[0] in reverse order.
	__asm__ __volatile__ (
	    "1:\n"
	    "ld __tmp_reg__,-Z\n"
	    "st X+,__tmp_reg__\n"
	    "dec %2\n"
	    "brne 1b\n"
	    : : "x"(k), "z"(key + len), "r"(8)
	);

	// Expand the key to the full key schedule.
	__asm__ __volatile__ (
	    "1:\n"
	    // l[li_out] = (k[i] + rightRotate8_64(l[li_in])) ^ i;
	    "add %A1,%2\n"              // X = &(l[li_in])
	    "adc %B1,__zero_reg__\n"
	    "ld r15,X+\n"               // x = rightRotate8_64(l[li_in])
	    "ld r8,X+\n"
	    "ld r9,X+\n"
	    "ld r10,X+\n"
	    "ld r11,X+\n"
	    "ld r12,X+\n"
	    "ld r13,X+\n"
	    "ld r14,X+\n"

	    "ld r16,Z+\n"               // y = k[i]
	    "ld r17,Z+\n"
	    "ld r18,Z+\n"
	    "ld r19,Z+\n"
	    "ld r20,Z+\n"
	    "ld r21,Z+\n"
	    "ld r22,Z+\n"
	    "ld r23,Z+\n"

	    "add r8,r16\n"              // x += y
	    "adc r9,r17\n"
	    "adc r10,r18\n"
	    "adc r11,r19\n"
	    "adc r12,r20\n"
	    "adc r13,r21\n"
	    "adc r14,r22\n"
	    "adc r15,r23\n"

	    "eor r8,%4\n"               // x ^= i

	    // X = X - li_in + li_out
	    "ldi r24,8\n"               // li_in = li_in + 1
	    "add %2,r24\n"
	    "sub %A1,%2\n"              // return X to its initial value
	    "sbc %B1,__zero_reg__\n"
	    "ldi r25,0x1f\n"
	    "and %2,r25\n"              // li_in = li_in % 4
	    "add %A1,%3\n"              // X = &(l[li_out])
	    "adc %B1,__zero_reg__\n"

	    "st X+,r8\n"                // l[li_out] = x
	    "st X+,r9\n"
	    "st X+,r10\n"
	    "st X+,r11\n"
	    "st X+,r12\n"
	    "st X+,r13\n"
	    "st X+,r14\n"
	    "st X+,r15\n"

	    "add %3,r24\n"              // li_out = li_out + 1
	    "sub %A1,%3\n"              // return X to its initial value
	    "sbc %B1,__zero_reg__\n"
	    "and %3,r25\n"              // li_out = li_out % 4

	    // k[i + 1] = leftRotate3_64(k[i]) ^ l[li_out];
	    "lsl r16\n"                 // y = leftRotate1_64(y)
	    "rol r17\n"
	    "rol r18\n"
	    "rol r19\n"
	    "rol r20\n"
	    "rol r21\n"
	    "rol r22\n"
	    "rol r23\n"
	    "adc r16,__zero_reg__\n"

	    "lsl r16\n"                 // y = leftRotate1_64(y)
	    "rol r17\n"
	    "rol r18\n"
	    "rol r19\n"
	    "rol r20\n"
	    "rol r21\n"
	    "rol r22\n"
	    "rol r23\n"
	    "adc r16,__zero_reg__\n"

	    "lsl r16\n"                 // y = leftRotate1_64(y)
	    "rol r17\n"
	    "rol r18\n"
	    "rol r19\n"
	    "rol r20\n"
	    "rol r21\n"
	    "rol r22\n"
	    "rol r23\n"
	    "adc r16,__zero_reg__\n"

	    "eor r16,r8\n"              // y ^= x
	    "eor r17,r9\n"
	    "eor r18,r10\n"
	    "eor r19,r11\n"
	    "eor r20,r12\n"
	    "eor r21,r13\n"
	    "eor r22,r14\n"
	    "eor r23,r15\n"

	    "st Z,r16\n"                // k[i + 1] = y
	    "std Z+1,r17\n"
	    "std Z+2,r18\n"
	    "std Z+3,r19\n"
	    "std Z+4,r20\n"
	    "std Z+5,r21\n"
	    "std Z+6,r22\n"
	    "std Z+7,r23\n"

	    // Loop
	    "inc %4\n"                  // ++i
	    "dec %5\n"                  // --rounds
	    "breq 2f\n"
	    "rjmp 1b\n"
	    "2:\n"

	    : : "z"(k), "x"(l),
	    "r"((uint8_t)0),                // initial value of li_in
	    "r"((uint8_t)((m - 1) * 8)),    // initial value of li_out
	    "r"(0),                         // initial value of i
	    "r"(rounds - 1)
	    :  "r8",  "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	    "r24", "r25"
	);
#else
	uint64_t l[4];
	uint8_t m;
	if (len == 32) {
		m = 4;
		unpack64(l[2], key);
		unpack64(l[1], key + 8);
		unpack64(l[0], key + 16);
		unpack64(k[0], key + 24);
	} else if (len == 24) {
		m = 3;
		unpack64(l[1], key);
		unpack64(l[0], key + 8);
		unpack64(k[0], key + 16);
	} else if (len == 16) {
		m = 2;
		unpack64(l[0], key);
		unpack64(k[0], key + 8);
	} else {
		return false;
	}
	rounds = 30 + m;
	uint8_t li_in = 0;
	uint8_t li_out = m - 1;
	for (uint8_t i = 0; i < (rounds - 1); ++i) {
		l[li_out] = (k[i] + rightRotate8_64(l[li_in])) ^ i;
		k[i + 1] = leftRotate3_64(k[i]) ^ l[li_out];
		if ((++li_in) >= m) {
			li_in = 0;
		}
		if ((++li_out) >= m) {
			li_out = 0;
		}
	}
#endif
	clean(l);
	return true;
}

void Speck::encryptBlock(uint8_t *output, const uint8_t *input)
{
#if USE_AVR_INLINE_ASM
	uint32_t xlow, xhigh, ylow, yhigh;

	// Unpack the input into the x and y variables, converting
	// from big-endian into little-endian in the process.
	__asm__ __volatile__ (
	    "ld %D1,Z\n"
	    "ldd %C1,Z+1\n"
	    "ldd %B1,Z+2\n"
	    "ldd %A1,Z+3\n"
	    "ldd %D0,Z+4\n"
	    "ldd %C0,Z+5\n"
	    "ldd %B0,Z+6\n"
	    "ldd %A0,Z+7\n"
	    "ldd %D3,Z+8\n"
	    "ldd %C3,Z+9\n"
	    "ldd %B3,Z+10\n"
	    "ldd %A3,Z+11\n"
	    "ldd %D2,Z+12\n"
	    "ldd %C2,Z+13\n"
	    "ldd %B2,Z+14\n"
	    "ldd %A2,Z+15\n"
	    : "=r"(xlow), "=r"(xhigh), "=r"(ylow), "=r"(yhigh)
	    : "z"(input)
	);

	// Perform all encryption rounds.  Z points to the key schedule.
	__asm__ __volatile__ (
	    "1:\n"
	    // x = (rightRotate8_64(x) + y) ^ *s++;
	    "mov __tmp_reg__,%A0\n"     // x = rightRotate8_64(x)
	    "mov %A0,%B0\n"
	    "mov %B0,%C0\n"
	    "mov %C0,%D0\n"
	    "mov %D0,%A1\n"
	    "mov %A1,%B1\n"
	    "mov %B1,%C1\n"
	    "mov %C1,%D1\n"
	    "mov %D1,__tmp_reg__\n"

	    "add %A0,%A2\n"             // x += y
	    "adc %B0,%B2\n"
	    "adc %C0,%C2\n"
	    "adc %D0,%D2\n"
	    "adc %A1,%A3\n"
	    "adc %B1,%B3\n"
	    "adc %C1,%C3\n"
	    "adc %D1,%D3\n"

	    "ld __tmp_reg__,Z+\n"       // x ^= *s++
	    "eor %A0,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "eor %B0,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "eor %C0,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "eor %D0,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "eor %A1,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "eor %B1,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "eor %C1,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "eor %D1,__tmp_reg__\n"

	    // y = leftRotate3_64(y) ^ x;
	    "lsl %A2\n"                 // y = leftRotate1_64(y)
	    "rol %B2\n"
	    "rol %C2\n"
	    "rol %D2\n"
	    "rol %A3\n"
	    "rol %B3\n"
	    "rol %C3\n"
	    "rol %D3\n"
	    "adc %A2,__zero_reg__\n"

	    "lsl %A2\n"                 // y = leftRotate1_64(y)
	    "rol %B2\n"
	    "rol %C2\n"
	    "rol %D2\n"
	    "rol %A3\n"
	    "rol %B3\n"
	    "rol %C3\n"
	    "rol %D3\n"
	    "adc %A2,__zero_reg__\n"

	    "lsl %A2\n"                 // y = leftRotate1_64(y)
	    "rol %B2\n"
	    "rol %C2\n"
	    "rol %D2\n"
	    "rol %A3\n"
	    "rol %B3\n"
	    "rol %C3\n"
	    "rol %D3\n"
	    "adc %A2,__zero_reg__\n"

	    "eor %A2,%A0\n"             // y ^= x
	    "eor %B2,%B0\n"
	    "eor %C2,%C0\n"
	    "eor %D2,%D0\n"
	    "eor %A3,%A1\n"
	    "eor %B3,%B1\n"
	    "eor %C3,%C1\n"
	    "eor %D3,%D1\n"

	    // Loop
	    "dec %5\n"                  // --round
	    "breq 2f\n"
	    "rjmp 1b\n"
	    "2:\n"
	    : "+r"(xlow), "+r"(xhigh), "+r"(ylow), "+r"(yhigh)
	    : "z"(k), "r"(rounds)
	);

	// Pack the results into the output and convert back to big-endian.
	__asm__ __volatile__ (
	    "st Z,%D1\n"
	    "std Z+1,%C1\n"
	    "std Z+2,%B1\n"
	    "std Z+3,%A1\n"
	    "std Z+4,%D0\n"
	    "std Z+5,%C0\n"
	    "std Z+6,%B0\n"
	    "std Z+7,%A0\n"
	    "std Z+8,%D3\n"
	    "std Z+9,%C3\n"
	    "std Z+10,%B3\n"
	    "std Z+11,%A3\n"
	    "std Z+12,%D2\n"
	    "std Z+13,%C2\n"
	    "std Z+14,%B2\n"
	    "std Z+15,%A2\n"
	    : : "r"(xlow), "r"(xhigh), "r"(ylow), "r"(yhigh), "z"(output)
	);
#else
	uint64_t x, y;
	const uint64_t *s = k;
	unpack64(x, input);
	unpack64(y, input + 8);
	for (uint8_t round = rounds; round > 0; --round, ++s) {
		x = (rightRotate8_64(x) + y) ^ s[0];
		y = leftRotate3_64(y) ^ x;
	}
	pack64(output, x);
	pack64(output + 8, y);
#endif
}

void Speck::decryptBlock(uint8_t *output, const uint8_t *input)
{
#if USE_AVR_INLINE_ASM
	uint32_t xlow, xhigh, ylow, yhigh;

	// Unpack the input into the x and y variables, converting
	// from big-endian into little-endian in the process.
	__asm__ __volatile__ (
	    "ld %D1,Z\n"
	    "ldd %C1,Z+1\n"
	    "ldd %B1,Z+2\n"
	    "ldd %A1,Z+3\n"
	    "ldd %D0,Z+4\n"
	    "ldd %C0,Z+5\n"
	    "ldd %B0,Z+6\n"
	    "ldd %A0,Z+7\n"
	    "ldd %D3,Z+8\n"
	    "ldd %C3,Z+9\n"
	    "ldd %B3,Z+10\n"
	    "ldd %A3,Z+11\n"
	    "ldd %D2,Z+12\n"
	    "ldd %C2,Z+13\n"
	    "ldd %B2,Z+14\n"
	    "ldd %A2,Z+15\n"
	    : "=r"(xlow), "=r"(xhigh), "=r"(ylow), "=r"(yhigh)
	    : "z"(input)
	);

	// Perform all decryption rounds.  Z points to the end of key schedule.
	__asm__ __volatile__ (
	    "1:\n"
	    // y = rightRotate3_64(x ^ y);
	    "eor %A2,%A0\n"             // y ^= x
	    "eor %B2,%B0\n"
	    "eor %C2,%C0\n"
	    "eor %D2,%D0\n"
	    "eor %A3,%A1\n"
	    "eor %B3,%B1\n"
	    "eor %C3,%C1\n"
	    "eor %D3,%D1\n"

	    "bst %A2,0\n"               // y = rightRotate1_64(y)
	    "ror %D3\n"
	    "ror %C3\n"
	    "ror %B3\n"
	    "ror %A3\n"
	    "ror %D2\n"
	    "ror %C2\n"
	    "ror %B2\n"
	    "ror %A2\n"
	    "bld %D3,7\n"

	    "bst %A2,0\n"               // y = rightRotate1_64(y)
	    "ror %D3\n"
	    "ror %C3\n"
	    "ror %B3\n"
	    "ror %A3\n"
	    "ror %D2\n"
	    "ror %C2\n"
	    "ror %B2\n"
	    "ror %A2\n"
	    "bld %D3,7\n"

	    "bst %A2,0\n"               // y = rightRotate1_64(y)
	    "ror %D3\n"
	    "ror %C3\n"
	    "ror %B3\n"
	    "ror %A3\n"
	    "ror %D2\n"
	    "ror %C2\n"
	    "ror %B2\n"
	    "ror %A2\n"
	    "bld %D3,7\n"

	    // x = leftRotate8_64((x ^ *s--) - y);
	    "ld __tmp_reg__,-Z\n"       // x ^= *s--
	    "eor %D1,__tmp_reg__\n"
	    "ld __tmp_reg__,-Z\n"
	    "eor %C1,__tmp_reg__\n"
	    "ld __tmp_reg__,-Z\n"
	    "eor %B1,__tmp_reg__\n"
	    "ld __tmp_reg__,-Z\n"
	    "eor %A1,__tmp_reg__\n"
	    "ld __tmp_reg__,-Z\n"
	    "eor %D0,__tmp_reg__\n"
	    "ld __tmp_reg__,-Z\n"
	    "eor %C0,__tmp_reg__\n"
	    "ld __tmp_reg__,-Z\n"
	    "eor %B0,__tmp_reg__\n"
	    "ld __tmp_reg__,-Z\n"
	    "eor %A0,__tmp_reg__\n"

	    "sub %A0,%A2\n"             // x -= y
	    "sbc %B0,%B2\n"
	    "sbc %C0,%C2\n"
	    "sbc %D0,%D2\n"
	    "sbc %A1,%A3\n"
	    "sbc %B1,%B3\n"
	    "sbc %C1,%C3\n"
	    "sbc %D1,%D3\n"

	    "mov __tmp_reg__,%D1\n"     // x = lefRotate8_64(x)
	    "mov %D1,%C1\n"
	    "mov %C1,%B1\n"
	    "mov %B1,%A1\n"
	    "mov %A1,%D0\n"
	    "mov %D0,%C0\n"
	    "mov %C0,%B0\n"
	    "mov %B0,%A0\n"
	    "mov %A0,__tmp_reg__\n"

	    // Loop
	    "dec %5\n"                  // --round
	    "breq 2f\n"
	    "rjmp 1b\n"
	    "2:\n"
	    : "+r"(xlow), "+r"(xhigh), "+r"(ylow), "+r"(yhigh)
	    : "z"(k + rounds), "r"(rounds)
	);

	// Pack the results into the output and convert back to big-endian.
	__asm__ __volatile__ (
	    "st Z,%D1\n"
	    "std Z+1,%C1\n"
	    "std Z+2,%B1\n"
	    "std Z+3,%A1\n"
	    "std Z+4,%D0\n"
	    "std Z+5,%C0\n"
	    "std Z+6,%B0\n"
	    "std Z+7,%A0\n"
	    "std Z+8,%D3\n"
	    "std Z+9,%C3\n"
	    "std Z+10,%B3\n"
	    "std Z+11,%A3\n"
	    "std Z+12,%D2\n"
	    "std Z+13,%C2\n"
	    "std Z+14,%B2\n"
	    "std Z+15,%A2\n"
	    : : "r"(xlow), "r"(xhigh), "r"(ylow), "r"(yhigh), "z"(output)
	);
#else
	uint64_t x, y;
	const uint64_t *s = k + rounds - 1;
	unpack64(x, input);
	unpack64(y, input + 8);
	for (uint8_t round = rounds; round > 0; --round, --s) {
		y = rightRotate3_64(x ^ y);
		x = leftRotate8_64((x ^ s[0]) - y);
	}
	pack64(output, x);
	pack64(output + 8, y);
#endif
}

void Speck::clear()
{
	clean(k);
}
