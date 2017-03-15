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

#include "SpeckTiny.h"
#include "Crypto.h"
#include "utility/RotateUtil.h"
#include "utility/EndianUtil.h"
#include <string.h>

/**
 * \class SpeckTiny SpeckTiny.h <SpeckTiny.h>
 * \brief Speck block cipher with a 128-bit block size (tiny-memory version).
 *
 * This class differs from the Speck class in the following ways:
 *
 * \li RAM requirements are vastly reduced.  The key (up to 256 bits) is
 * stored directly and then expanded to the full key schedule round by round.
 * The setKey() method is very fast because of this.
 * \li Performance of encryptBlock() is slower than for Speck due to
 * expanding the key on the fly rather than ahead of time.
 * \li The decryptBlock() function is not supported, which means that CBC
 * mode cannot be used but the CTR, CFB, OFB, EAX, and GCM modes can be used.
 *
 * This class is useful when RAM is at a premium, CBC mode is not required,
 * and reduced encryption performance is not a hindrance to the application.
 * Even though the performance of encryptBlock() is reduced, this class is
 * still faster than AES with equivalent key sizes.
 *
 * The companion SpeckSmall class supports decryptBlock() at the cost of
 * some additional memory and slower setKey() times.
 *
 * See the documentation for the Speck class for more information on the
 * Speck family of block ciphers.
 *
 * References: https://en.wikipedia.org/wiki/Speck_%28cipher%29,
 * http://eprint.iacr.org/2013/404
 *
 * \sa Speck, SpeckSmall
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
 * \brief Constructs a tiny-memory Speck block cipher with no initial key.
 *
 * This constructor must be followed by a call to setKey() before the
 * block cipher can be used for encryption.
 */
SpeckTiny::SpeckTiny()
	: rounds(32)
{
}

SpeckTiny::~SpeckTiny()
{
	clean(k);
}

size_t SpeckTiny::blockSize() const
{
	return 16;
}

size_t SpeckTiny::keySize() const
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

bool SpeckTiny::setKey(const uint8_t *key, size_t len)
{
#if USE_AVR_INLINE_ASM
	// Determine the number of rounds to use and validate the key length.
	if (len == 32) {
		rounds = 34;
	} else if (len == 24) {
		rounds = 33;
	} else if (len == 16) {
		rounds = 32;
	} else {
		return false;
	}

	// Copy the bytes of the key into the "k" array in reverse order to
	// convert big endian into little-endian.
	__asm__ __volatile__ (
	    "1:\n"
	    "ld __tmp_reg__,-Z\n"
	    "st X+,__tmp_reg__\n"
	    "dec %2\n"
	    "brne 1b\n"
	    : : "x"(k), "z"(key + len), "r"(len)
	);
#else
	if (len == 32) {
		rounds = 34;
		unpack64(k[3], key);
		unpack64(k[2], key + 8);
		unpack64(k[1], key + 16);
		unpack64(k[0], key + 24);
	} else if (len == 24) {
		rounds = 33;
		unpack64(k[2], key);
		unpack64(k[1], key + 8);
		unpack64(k[0], key + 16);
	} else if (len == 16) {
		rounds = 32;
		unpack64(k[1], key);
		unpack64(k[0], key + 8);
	} else {
		return false;
	}
#endif
	return true;
}

void SpeckTiny::encryptBlock(uint8_t *output, const uint8_t *input)
{
#if USE_AVR_INLINE_ASM
	uint64_t l[4];
	uint32_t xlow, xhigh, ylow, yhigh;
	uint32_t slow, shigh;
	uint8_t li_in = 0;
	uint8_t li_out = (rounds - 31) * 8;

	// Copy the "k" array into "s" and the "l" array.
	__asm__ __volatile__ (
	    "ldd r25,%4\n"          // r25 = li_out

	    "ld __tmp_reg__,Z+\n"
	    "std %A0,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "std %B0,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "std %C0,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "std %D0,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "std %A1,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "std %B1,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "std %C1,__tmp_reg__\n"
	    "ld __tmp_reg__,Z+\n"
	    "std %D1,__tmp_reg__\n"

	    "1:\n"                  // l[0..] = k[1..]
	    "ld __tmp_reg__,Z+\n"
	    "st X+,__tmp_reg__\n"
	    "dec r25\n"
	    "brne 1b\n"
	    : "=Q"(slow), "=Q"(shigh)
	    : "z"(k), "x"(l), "Q"(li_out)
	    : "r25"
	);

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

	// Perform all encryption rounds while expanding the key schedule in-place.
	__asm__ __volatile__ (
	    "mov r23,__zero_reg__\n"    // i = 0
	    "1:\n"

	    // Adjust x and y for this round using the key schedule word s.

	    // x = (rightRotate8_64(x) + y) ^ s;
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

	    "ldd __tmp_reg__,%A4\n"     // x ^= s
	    "eor %A0,__tmp_reg__\n"
	    "ldd __tmp_reg__,%B4\n"
	    "eor %B0,__tmp_reg__\n"
	    "ldd __tmp_reg__,%C4\n"
	    "eor %C0,__tmp_reg__\n"
	    "ldd __tmp_reg__,%D4\n"
	    "eor %D0,__tmp_reg__\n"
	    "ldd __tmp_reg__,%A5\n"
	    "eor %A1,__tmp_reg__\n"
	    "ldd __tmp_reg__,%B5\n"
	    "eor %B1,__tmp_reg__\n"
	    "ldd __tmp_reg__,%C5\n"
	    "eor %C1,__tmp_reg__\n"
	    "ldd __tmp_reg__,%D5\n"
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

	    // On the last round we don't need to compute s so we
	    // can exit early here if (i + 1) == rounds.
	    "mov __tmp_reg__,r23\n"     // temp = i + 1
	    "inc __tmp_reg__\n"
	    "cp __tmp_reg__,%9\n"       // if (temp == rounds) ...
	    "brne 2f\n"
	    "rjmp 3f\n"
	    "2:\n"

	    // Save x and y on the stack so we can reuse registers for t and s.
	    "push %A0\n"
	    "push %B0\n"
	    "push %C0\n"
	    "push %D0\n"
	    "push %A1\n"
	    "push %B1\n"
	    "push %C1\n"
	    "push %D1\n"
	    "push %A2\n"
	    "push %B2\n"
	    "push %C2\n"
	    "push %D2\n"
	    "push %A3\n"
	    "push %B3\n"
	    "push %C3\n"
	    "push %D3\n"

	    // Compute the key schedule word s for the next round.

	    // l[li_out] = (s + rightRotate8_64(l[li_in])) ^ i;
	    "ldd r24,%6\n"              // Z = &(l[li_in])
	    "add %A8,r24\n"
	    "adc %B8,__zero_reg__\n"

	    "ld %D1,Z+\n"               // t = rightRotate8_64(l[li_in])
	    "ld %A0,Z+\n"
	    "ld %B0,Z+\n"
	    "ld %C0,Z+\n"
	    "ld %D0,Z+\n"
	    "ld %A1,Z+\n"
	    "ld %B1,Z+\n"
	    "ld %C1,Z+\n"

	    "ldd %A2,%A4\n"             // load s
	    "ldd %B2,%B4\n"
	    "ldd %C2,%C4\n"
	    "ldd %D2,%D4\n"
	    "ldd %A3,%A5\n"
	    "ldd %B3,%B5\n"
	    "ldd %C3,%C5\n"
	    "ldd %D3,%D5\n"

	    "add %A0,%A2\n"             // t += s
	    "adc %B0,%B2\n"
	    "adc %C0,%C2\n"
	    "adc %D0,%D2\n"
	    "adc %A1,%A3\n"
	    "adc %B1,%B3\n"
	    "adc %C1,%C3\n"
	    "adc %D1,%D3\n"

	    "eor %A0,r23\n"             // t ^= i

	    // Z = Z - li_in + li_out
	    "ldi r25,8\n"               // li_in = li_in + 1
	    "add r24,r25\n"
	    "sub %A8,r24\n"             // return Z to its initial value
	    "sbc %B8,__zero_reg__\n"
	    "andi r24,0x1f\n"           // li_in = li_in % 4
	    "std %6,r24\n"
	    "ldd r24,%7\n"              // Z = &(l[li_out])
	    "add %A8,r24\n"
	    "adc %B8,__zero_reg__\n"

	    "st Z+,%A0\n"               // l[li_out] = t
	    "st Z+,%B0\n"
	    "st Z+,%C0\n"
	    "st Z+,%D0\n"
	    "st Z+,%A1\n"
	    "st Z+,%B1\n"
	    "st Z+,%C1\n"
	    "st Z+,%D1\n"

	    "add r24,r25\n"             // li_out = li_out + 1
	    "sub %A8,r24\n"             // return Z to its initial value
	    "sbc %B8,__zero_reg__\n"
	    "andi r24,0x1f\n"           // li_out = li_out % 4
	    "std %7,r24\n"

	    // s = leftRotate3_64(s) ^ l[li_out];
	    "lsl %A2\n"                 // s = leftRotate1_64(s)
	    "rol %B2\n"
	    "rol %C2\n"
	    "rol %D2\n"
	    "rol %A3\n"
	    "rol %B3\n"
	    "rol %C3\n"
	    "rol %D3\n"
	    "adc %A2,__zero_reg__\n"

	    "lsl %A2\n"                 // s = leftRotate1_64(s)
	    "rol %B2\n"
	    "rol %C2\n"
	    "rol %D2\n"
	    "rol %A3\n"
	    "rol %B3\n"
	    "rol %C3\n"
	    "rol %D3\n"
	    "adc %A2,__zero_reg__\n"

	    "lsl %A2\n"                 // s = leftRotate1_64(s)
	    "rol %B2\n"
	    "rol %C2\n"
	    "rol %D2\n"
	    "rol %A3\n"
	    "rol %B3\n"
	    "rol %C3\n"
	    "rol %D3\n"
	    "adc %A2,__zero_reg__\n"

	    "eor %A2,%A0\n"             // s ^= l[li_out]
	    "eor %B2,%B0\n"
	    "eor %C2,%C0\n"
	    "eor %D2,%D0\n"
	    "eor %A3,%A1\n"
	    "eor %B3,%B1\n"
	    "eor %C3,%C1\n"
	    "eor %D3,%D1\n"

	    "std %A4,%A2\n"             // store s
	    "std %B4,%B2\n"
	    "std %C4,%C2\n"
	    "std %D4,%D2\n"
	    "std %A5,%A3\n"
	    "std %B5,%B3\n"
	    "std %C5,%C3\n"
	    "std %D5,%D3\n"

	    // Pop registers from the stack to recover the x and y values.
	    "pop %D3\n"
	    "pop %C3\n"
	    "pop %B3\n"
	    "pop %A3\n"
	    "pop %D2\n"
	    "pop %C2\n"
	    "pop %B2\n"
	    "pop %A2\n"
	    "pop %D1\n"
	    "pop %C1\n"
	    "pop %B1\n"
	    "pop %A1\n"
	    "pop %D0\n"
	    "pop %C0\n"
	    "pop %B0\n"
	    "pop %A0\n"

	    // Bottom of the loop.
	    "inc r23\n"
	    "rjmp 1b\n"
	    "3:\n"

	    : "+r"(xlow), "+r"(xhigh), "+r"(ylow), "+r"(yhigh),
	    "+Q"(slow), "+Q"(shigh), "+Q"(li_in), "+Q"(li_out)
	    : "z"(l), "r"(rounds)
	    : "r23", "r24", "r25"
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
	uint64_t l[4];
	uint64_t x, y, s;
	uint8_t round;
	uint8_t li_in = 0;
	uint8_t li_out = rounds - 31;
	uint8_t i = 0;

	// Copy the input block into the work registers.
	unpack64(x, input);
	unpack64(y, input + 8);

	// Prepare the key schedule.
	memcpy(l, k + 1, li_out * sizeof(uint64_t));
	s = k[0];

	// Perform all encryption rounds except the last.
	for (round = rounds - 1; round > 0; --round, ++i) {
		// Perform the round with the current key schedule word.
		x = (rightRotate8_64(x) + y) ^ s;
		y = leftRotate3_64(y) ^ x;

		// Calculate the next key schedule word.
		l[li_out] = (s + rightRotate8_64(l[li_in])) ^ i;
		s = leftRotate3_64(s) ^ l[li_out];
		li_in = (li_in + 1) & 0x03;
		li_out = (li_out + 1) & 0x03;
	}

	// Perform the final round and copy to the output.
	x = (rightRotate8_64(x) + y) ^ s;
	y = leftRotate3_64(y) ^ x;
	pack64(output, x);
	pack64(output + 8, y);
#endif
}

void SpeckTiny::decryptBlock(uint8_t *output, const uint8_t *input)
{
	// Decryption is not supported by SpeckTiny.  Use SpeckSmall instead.
}

void SpeckTiny::clear()
{
	clean(k);
}
