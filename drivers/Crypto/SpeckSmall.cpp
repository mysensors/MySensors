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

#include "SpeckSmall.h"
#include "Crypto.h"
#include "utility/RotateUtil.h"
#include "utility/EndianUtil.h"
#include <string.h>

/**
 * \class SpeckSmall SpeckSmall.h <SpeckSmall.h>
 * \brief Speck block cipher with a 128-bit block size (small-memory version).
 *
 * This class differs from the Speck class in that the RAM requirements are
 * vastly reduced.  The key schedule is expanded round by round instead of
 * being generated and stored by setKey().  The performance of encryption
 * and decryption is slightly less because of this.
 *
 * This class is useful when RAM is at a premium and reduced encryption
 * performance is not a hindrance to the application.  Even though the
 * performance is reduced, this class is still faster than AES with
 * equivalent key sizes.
 *
 * The companion SpeckTiny class uses even less RAM but only supports the
 * encryptBlock() operation.  Block cipher modes like CTR, EAX, and GCM
 * do not need the decryptBlock() operation, so SpeckTiny may be a better
 * option than SpeckSmall for many applications.
 *
 * See the documentation for the Speck class for more information on the
 * Speck family of block ciphers.
 *
 * References: https://en.wikipedia.org/wiki/Speck_%28cipher%29,
 * http://eprint.iacr.org/2013/404
 *
 * \sa Speck, SpeckTiny
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

/**
 * \brief Constructs a small-memory Speck block cipher with no initial key.
 *
 * This constructor must be followed by a call to setKey() before the
 * block cipher can be used for encryption or decryption.
 */
SpeckSmall::SpeckSmall()
{
}

SpeckSmall::~SpeckSmall()
{
	clean(l);
}

bool SpeckSmall::setKey(const uint8_t *key, size_t len)
{
	// Try setting the key for the forward encryption direction.
	if (!SpeckTiny::setKey(key, len)) {
		return false;
	}

#if USE_AVR_INLINE_ASM
	// Expand the key schedule to get the l and s values at the end
	// of the schedule, which will allow us to reverse it later.
	uint8_t mb = (rounds - 31) * 8;
	__asm__ __volatile__ (
	    "ld r16,Z+\n"               // s = k[0]
	    "ld r17,Z+\n"
	    "ld r18,Z+\n"
	    "ld r19,Z+\n"
	    "ld r20,Z+\n"
	    "ld r21,Z+\n"
	    "ld r22,Z+\n"
	    "ld r23,Z+\n"

	    "mov r24,%3\n"              // memcpy(l, k + 1, mb)
	    "3:\n"
	    "ld __tmp_reg__,Z+\n"
	    "st X+,__tmp_reg__\n"
	    "dec r24\n"
	    "brne 3b\n"
	    "sub %A1,%3\n"              // return X to its initial value
	    "sbc %B1,__zero_reg__\n"

	    "1:\n"

	    // l[li_out] = (s + rightRotate8_64(l[li_in])) ^ i;
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

	    "add r8,r16\n"              // x += s
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

	    // s = leftRotate3_64(s) ^ l[li_out];
	    "lsl r16\n"                 // s = leftRotate1_64(s)
	    "rol r17\n"
	    "rol r18\n"
	    "rol r19\n"
	    "rol r20\n"
	    "rol r21\n"
	    "rol r22\n"
	    "rol r23\n"
	    "adc r16,__zero_reg__\n"

	    "lsl r16\n"                 // s = leftRotate1_64(s)
	    "rol r17\n"
	    "rol r18\n"
	    "rol r19\n"
	    "rol r20\n"
	    "rol r21\n"
	    "rol r22\n"
	    "rol r23\n"
	    "adc r16,__zero_reg__\n"

	    "lsl r16\n"                 // s = leftRotate1_64(s)
	    "rol r17\n"
	    "rol r18\n"
	    "rol r19\n"
	    "rol r20\n"
	    "rol r21\n"
	    "rol r22\n"
	    "rol r23\n"
	    "adc r16,__zero_reg__\n"

	    "eor r16,r8\n"              // s ^= x
	    "eor r17,r9\n"
	    "eor r18,r10\n"
	    "eor r19,r11\n"
	    "eor r20,r12\n"
	    "eor r21,r13\n"
	    "eor r22,r14\n"
	    "eor r23,r15\n"

	    // Loop
	    "inc %4\n"                  // ++i
	    "dec %5\n"                  // --rounds
	    "breq 2f\n"
	    "rjmp 1b\n"
	    "2:\n"

	    "add %A1,%3\n"              // X = &(l[li_out])
	    "adc %B1,__zero_reg__\n"
	    "st X+,r16\n"               // l[li_out] = s
	    "st X+,r17\n"
	    "st X+,r18\n"
	    "st X+,r19\n"
	    "st X+,r20\n"
	    "st X+,r21\n"
	    "st X+,r22\n"
	    "st X+,r23\n"

	    : : "z"(k), "x"(l),
	    "r"((uint8_t)0),                // initial value of li_in
	    "r"((uint8_t)mb),               // initial value of li_out
	    "r"(0),                         // initial value of i
	    "r"(rounds - 1)
	    :  "r8",  "r9", "r10", "r11", "r12", "r13", "r14", "r15",
	    "r16", "r17", "r18", "r19", "r20", "r21", "r22", "r23",
	    "r24", "r25"
	);
	return true;
#else
	// Expand the key schedule to get the l and s values at the end
	// of the schedule, which will allow us to reverse it later.
	uint8_t m = rounds - 30;
	uint8_t li_in = 0;
	uint8_t li_out = m - 1;
	uint64_t s = k[0];
	memcpy(l, k + 1, (m - 1) * sizeof(uint64_t));
	for (uint8_t i = 0; i < (rounds - 1); ++i) {
		l[li_out] = (s + rightRotate8_64(l[li_in])) ^ i;
		s = leftRotate3_64(s) ^ l[li_out];
		li_in = (li_in + 1) & 0x03;
		li_out = (li_out + 1) & 0x03;
	}

	// Save the final s value in the l array so that we can recover it later.
	l[li_out] = s;
	return true;
#endif
}

void SpeckSmall::decryptBlock(uint8_t *output, const uint8_t *input)
{
#if USE_AVR_INLINE_ASM
	uint64_t l[4];
	uint32_t xlow, xhigh, ylow, yhigh;
	uint32_t slow, shigh;
	uint8_t li_in = (rounds + 3) & 0x03;
	uint8_t li_out = (((rounds - 31) + li_in) & 0x03) * 8;
	li_in *= 8;

	// Prepare to expand the key schedule.
	__asm__ __volatile__ (
	    "add r30,%4\n"              // Z = &(this->l[li_out])
	    "adc r31,__zero_reg__\n"
	    "ld __tmp_reg__,Z\n"        // s = this->l[li_out]
	    "std %A0,__tmp_reg__\n"
	    "ldd __tmp_reg__,Z+1\n"
	    "std %B0,__tmp_reg__\n"
	    "ldd __tmp_reg__,Z+2\n"
	    "std %C0,__tmp_reg__\n"
	    "ldd __tmp_reg__,Z+3\n"
	    "std %D0,__tmp_reg__\n"
	    "ldd __tmp_reg__,Z+4\n"
	    "std %A1,__tmp_reg__\n"
	    "ldd __tmp_reg__,Z+5\n"
	    "std %B1,__tmp_reg__\n"
	    "ldd __tmp_reg__,Z+6\n"
	    "std %C1,__tmp_reg__\n"
	    "ldd __tmp_reg__,Z+7\n"
	    "std %D1,__tmp_reg__\n"
	    "sub r30,%4\n"              // Point Z back to the start of this->l.
	    "sbc r31,__zero_reg__\n"

	    "ldi r25,32\n"              // Copy the entire this->l array into l.
	    "1:\n"
	    "ld __tmp_reg__,Z+\n"
	    "st X+,__tmp_reg__\n"
	    "dec r25\n"
	    "brne 1b\n"
	    : "=Q"(slow), "=Q"(shigh)
	    : "z"(this->l), "x"(l), "r"(li_out)
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

	// Perform all decryption rounds while expanding the key schedule in-place.
	__asm__ __volatile__ (
	    "mov r23,%9\n"              // i = rounds - 1
	    "dec r23\n"
	    "1:\n"

	    // Adjust x and y for this round using the key schedule word s.

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

	    // x = leftRotate8_64((x ^ s) - y);
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

	    // On the last round we don't need to compute s so we
	    // can exit early here if i == 0.
	    "or r23,r23\n"              // if (i == 0)
	    "brne 2f\n"
	    "rjmp 3f\n"
	    "2:\n"
	    "dec r23\n"                 // --i

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

	    // li_out = (li_out + 3) & 0x03;
	    "ldd r24,%7\n"
	    "ldi r25,24\n"
	    "add r24,r25\n"
	    "andi r24,0x1f\n"
	    "std %7,r24\n"

	    // s = rightRotate3_64(s ^ l[li_out]);
	    "add %A8,r24\n"             // Z = &(l[li_out])
	    "adc %B8,__zero_reg__\n"

	    "ld %A0,Z\n"                // t = l[li_out]
	    "ldd %B0,Z+1\n"
	    "ldd %C0,Z+2\n"
	    "ldd %D0,Z+3\n"
	    "ldd %A1,Z+4\n"
	    "ldd %B1,Z+5\n"
	    "ldd %C1,Z+6\n"
	    "ldd %D1,Z+7\n"

	    "ldd %A2,%A4\n"             // load s
	    "ldd %B2,%B4\n"
	    "ldd %C2,%C4\n"
	    "ldd %D2,%D4\n"
	    "ldd %A3,%A5\n"
	    "ldd %B3,%B5\n"
	    "ldd %C3,%C5\n"
	    "ldd %D3,%D5\n"

	    "eor %A2,%A0\n"             // s ^= t
	    "eor %B2,%B0\n"
	    "eor %C2,%C0\n"
	    "eor %D2,%D0\n"
	    "eor %A3,%A1\n"
	    "eor %B3,%B1\n"
	    "eor %C3,%C1\n"
	    "eor %D3,%D1\n"

	    "bst %A2,0\n"               // s = rightRotate1_64(s)
	    "ror %D3\n"
	    "ror %C3\n"
	    "ror %B3\n"
	    "ror %A3\n"
	    "ror %D2\n"
	    "ror %C2\n"
	    "ror %B2\n"
	    "ror %A2\n"
	    "bld %D3,7\n"

	    "bst %A2,0\n"               // s = rightRotate1_64(s)
	    "ror %D3\n"
	    "ror %C3\n"
	    "ror %B3\n"
	    "ror %A3\n"
	    "ror %D2\n"
	    "ror %C2\n"
	    "ror %B2\n"
	    "ror %A2\n"
	    "bld %D3,7\n"

	    "bst %A2,0\n"               // s = rightRotate1_64(s)
	    "ror %D3\n"
	    "ror %C3\n"
	    "ror %B3\n"
	    "ror %A3\n"
	    "ror %D2\n"
	    "ror %C2\n"
	    "ror %B2\n"
	    "ror %A2\n"
	    "bld %D3,7\n"

	    "sub %A8,r24\n"             // Z -= li_out
	    "sbc %B8,__zero_reg__\n"

	    // li_in = (li_in + 3) & 0x03;
	    "ldd r24,%6\n"
	    "add r24,r25\n"
	    "andi r24,0x1f\n"
	    "std %6,r24\n"

	    // l[li_in] = leftRotate8_64((l[li_out] ^ i) - s);
	    "add %A8,r24\n"             // Z = &(l[li_in])
	    "adc %B8,__zero_reg__\n"

	    "eor %A0,r23\n"             // t ^= i

	    "sub %A0,%A2\n"             // t -= s
	    "sbc %B0,%B2\n"
	    "sbc %C0,%C2\n"
	    "sbc %D0,%D2\n"
	    "sbc %A1,%A3\n"
	    "sbc %B1,%B3\n"
	    "sbc %C1,%C3\n"
	    "sbc %D1,%D3\n"

	    "st Z,%D1\n"                // l[li_in] = leftRotate8_64(t)
	    "std Z+1,%A0\n"
	    "std Z+2,%B0\n"
	    "std Z+3,%C0\n"
	    "std Z+4,%D0\n"
	    "std Z+5,%A1\n"
	    "std Z+6,%B1\n"
	    "std Z+7,%C1\n"

	    "sub %A8,r24\n"             // Z -= li_in
	    "sbc %B8,__zero_reg__\n"

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
	uint8_t li_in = (rounds + 3) & 0x03;
	uint8_t li_out = ((rounds - 31) + li_in) & 0x03;

	// Prepare the key schedule, starting at the end.
	for (round = li_in; round != li_out; round = (round + 1) & 0x03) {
		l[round] = this->l[round];
	}
	s = this->l[li_out];

	// Unpack the input and convert from big-endian.
	unpack64(x, input);
	unpack64(y, input + 8);

	// Perform all decryption rounds except the last while
	// expanding the decryption schedule on the fly.
	for (uint8_t round = rounds - 1; round > 0; --round) {
		// Decrypt using the current round key.
		y = rightRotate3_64(x ^ y);
		x = leftRotate8_64((x ^ s) - y);

		// Generate the round key for the previous round.
		li_in = (li_in + 3) & 0x03;
		li_out = (li_out + 3) & 0x03;
		s = rightRotate3_64(s ^ l[li_out]);
		l[li_in] = leftRotate8_64((l[li_out] ^ (round - 1)) - s);
	}

	// Perform the final decryption round.
	y = rightRotate3_64(x ^ y);
	x = leftRotate8_64((x ^ s) - y);

	// Pack the output and convert to big-endian.
	pack64(output, x);
	pack64(output + 8, y);
#endif
}

void SpeckSmall::clear()
{
	SpeckTiny::clear();
	clean(l);
}
