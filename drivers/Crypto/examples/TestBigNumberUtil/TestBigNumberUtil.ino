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

/*
This example runs tests on the utility functions in BigNumberUtil.
*/

#include <Crypto.h>
#include <BigNumberUtil.h>
#include <utility/ProgMemUtil.h>
#include <string.h>

#define NUM_SIZE_512BIT (64 / sizeof(limb_t))
#define LIMB_BITS       (sizeof(limb_t) * 8)

// Convert a decimal string in program memory into a big number.
void fromString(limb_t *x, size_t xsize, const char *str)
{
	uint8_t ch;
	size_t posn;
	memset(x, 0, sizeof(limb_t) * xsize);
	while ((ch = pgm_read_byte((uint8_t *)str)) != '\0') {
		if (ch >= '0' && ch <= '9') {
			// Quick and simple method to multiply by 10 and add the new digit.
			dlimb_t carry = ch - '0';
			for (posn = 0; posn < xsize; ++posn) {
				carry += ((dlimb_t)x[posn]) * 10U;
				x[posn] = (limb_t)carry;
				carry >>= LIMB_BITS;
			}
		}
		++str;
	}
}

// Convert a decimal string in program memory into a byte array.
void bytesFromString(uint8_t *x, size_t xsize, const char *str)
{
	uint8_t ch;
	size_t posn;
	memset(x, 0, xsize);
	while ((ch = pgm_read_byte((uint8_t *)str)) != '\0') {
		if (ch >= '0' && ch <= '9') {
			// Quick and simple method to multiply by 10 and add the new digit.
			uint16_t carry = ch - '0';
			for (posn = 0; posn < xsize; ++posn) {
				carry += ((uint16_t)x[posn]) * 10U;
				x[posn] = (uint8_t)carry;
				carry >>= 8;
			}
		}
		++str;
	}
}

// Compare two big numbers.  Returns -1, 0, or 1.
int compare(const limb_t *x, size_t xsize, const limb_t *y, size_t ysize)
{
	limb_t a, b;
	while (xsize > 0 || ysize > 0) {
		if (xsize > ysize) {
			--xsize;
			a = x[xsize];
			b = 0;
		} else if (ysize > xsize) {
			--ysize;
			a = 0;
			b = y[ysize];
		} else {
			--xsize;
			--ysize;
			a = x[xsize];
			b = y[ysize];
		}
		if (a < b) {
			return -1;
		} else if (a > b) {
			return 1;
		}
	}
	return 0;
}

// Compare two numbers where one is a decimal string.  Returns -1, 0, or 1.
int compare(const limb_t *x, size_t xsize, const char *y)
{
	limb_t val[NUM_SIZE_512BIT];
	fromString(val, NUM_SIZE_512BIT, y);
	return compare(x, xsize, val, NUM_SIZE_512BIT);
}

// Prints a number in hexadecimal.
void printNumber(const char *name, const limb_t *x, size_t xsize)
{
	static const char hexchars[] = "0123456789ABCDEF";
	Serial.print(name);
	Serial.print(" = ");
	for (size_t posn = 0; posn < xsize; ++posn) {
		for (uint8_t bit = LIMB_BITS; bit > 0; ) {
			bit -= 4;
			Serial.print(hexchars[(x[xsize - 1 - posn] >> bit) & 0x0F]);
		}
		Serial.print(' ');
	}
	Serial.println();
}

// Standard numbers that are useful in big number tests.
char const num_0[] PROGMEM = "0";
char const num_1[] PROGMEM = "1";
char const num_2[] PROGMEM = "2";
char const num_4[] PROGMEM = "4";
char const num_5[] PROGMEM = "5";
char const num_128[] PROGMEM = "128";
char const num_256[] PROGMEM = "256";
char const num_2_64_m7[] PROGMEM = "18446744073709551609"; // 2^64 - 7
char const num_2_129_m5[] PROGMEM = "680564733841876926926749214863536422907"; // 2^129 - 5
char const num_pi_77[] PROGMEM =
    "31415926535897932384626433832795028841971693993751058209749445923078164062862"; // 77 digits of pi
char const num_pi_154[] PROGMEM =
    "3141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481"; // 154 digits of pi
char const num_2_255_m253[] PROGMEM =
    "57896044618658097711785492504343953926634992332820282019728792003956564819715"; // 2^255 - 253
char const num_2_255_m20[] PROGMEM =
    "57896044618658097711785492504343953926634992332820282019728792003956564819948"; // 2^255 - 20
char const num_2_255_m19[] PROGMEM =
    "57896044618658097711785492504343953926634992332820282019728792003956564819949"; // 2^255 - 19
char const num_2_255_m19_x2[] PROGMEM =
    "115792089237316195423570985008687907853269984665640564039457584007913129639898"; // (2^255 - 19) * 2
char const num_a24[] PROGMEM = "121665";
char const num_2_512_m19[] PROGMEM =
    "13407807929942597099574024998205846127479365820592393377723561443721764030073546976801874298166903427690031858186486050853753882811946569946433649006084077"; // (2^512 - 19)

// Table of useful numbers.
const char * const numbers[] = {
	num_0,
	num_1,
	num_2,
	num_4,
	num_5,
	num_128,
	num_256,
	num_2_64_m7,
	num_2_129_m5,
	num_pi_77,
	num_2_255_m253,
	num_2_255_m20,
	num_pi_154,
	num_2_512_m19,
	0
};
#define numbers_count   ((sizeof(numbers) / sizeof(numbers[0])) - 1)

#define foreach_number(var) \
	const char *var = numbers[0]; \
	for (unsigned index##var = 0; index##var < numbers_count; \
	        ++index##var, var = numbers[index##var])

// Print a decimal string from program memory.
void printProgMem(const char *str)
{
	uint8_t ch;
	while ((ch = pgm_read_byte((uint8_t *)str)) != '\0') {
		Serial.print((char)ch);
		++str;
	}
}

// Determine if an array consists of all zero bytes.
static bool isAllZero(const uint8_t *bytes, size_t size)
{
	while (size > 0) {
		if (*bytes++ != 0) {
			return false;
		}
		--size;
	}
	return true;
}

// Determine if an array consists of all 0xBA bytes.
static bool isAllBad(const uint8_t *bytes, size_t size)
{
	while (size > 0) {
		if (*bytes++ != 0xBA) {
			return false;
		}
		--size;
	}
	return true;
}

#if !BIGNUMBER_LIMB_8BIT

static bool isAllBad(const limb_t *limbs, size_t size)
{
	return isAllBad((uint8_t *)limbs, size * sizeof(limb_t));
}

#endif

// Truncate the limb representation of a number to a specific byte length.
static void truncateNumber(limb_t *limbs, size_t bytes)
{
	size_t posn = NUM_SIZE_512BIT * sizeof(limb_t);
	size_t posn2;
	limb_t mask;
	while (posn > bytes) {
		--posn;
		posn2 = posn % sizeof(limb_t);
		if (posn2 == 0) {
			mask = 0;
		} else if (posn2 == 1) {
			mask = 0x000000FF;
		} else if (posn2 == 2) {
			mask = 0x0000FFFF;
		}
#if BIGNUMBER_LIMB_64BIT
		else if (posn2 == 3) {
			mask = 0x00FFFFFF;
		} else if (posn2 == 4) {
			mask = 0xFFFFFFFF;
		} else if (posn2 == 5) {
			mask = 0xFFFFFFFFFF;
		} else if (posn2 == 6) {
			mask = 0xFFFFFFFFFFFF;
		} else {
			mask = 0xFFFFFFFFFFFFFF;
		}
#else
		else {
			mask = 0x00FFFFFF;
		}
#endif
		limbs[posn / sizeof(limb_t)] &= mask;
	}
}

// Test byte array pack and unpack operations.
void testPackUnpack(void)
{
	limb_t num[NUM_SIZE_512BIT];
	limb_t tnum[NUM_SIZE_512BIT];
	limb_t limbs[NUM_SIZE_512BIT];
	uint8_t bytes[64];
	uint8_t expected[64];
	size_t posn;
	uint8_t temp;

	foreach_number(x) {
		// What number are we on?
		Serial.print("pack ");
		printProgMem(x);
		Serial.print(": ");
		Serial.flush();
		bool ok = true;

		// Convert the number into limbs and bytes in a simple way.
		fromString(num, NUM_SIZE_512BIT, x);
		bytesFromString(expected, sizeof(expected), x);

		// Check packLE() and unpackLE() against the expected values.
		for (posn = 0; posn < 64; ++posn) {
			memset(bytes, 0xBA, sizeof(bytes));
			BigNumberUtil::packLE(bytes, posn, num, NUM_SIZE_512BIT);
			if (memcmp(bytes, expected, posn) != 0) {
				ok = false;
			}
			if (!isAllBad(bytes + posn, sizeof(bytes) - posn)) {
				ok = false;
			}
		}
		for (posn = 0; posn < NUM_SIZE_512BIT; ++posn) {
			BigNumberUtil::packLE(bytes, sizeof(bytes), num, posn);
			if (memcmp(bytes, expected, posn * sizeof(limb_t)) != 0) {
				ok = false;
			}
			if (!isAllZero(bytes + posn * sizeof(limb_t), sizeof(bytes) - posn * sizeof(limb_t))) {
				ok = false;
			}
		}
		for (posn = 0; posn < NUM_SIZE_512BIT; ++posn) {
			memset(limbs, 0xBA, sizeof(limbs));
			BigNumberUtil::unpackLE(limbs, posn, expected, sizeof(expected));
			if (memcmp(limbs, num, posn) != 0) {
				ok = false;
			}
			if (!isAllBad(limbs + posn, NUM_SIZE_512BIT - posn)) {
				ok = false;
			}
		}
		for (posn = 0; posn < 64; ++posn) {
			memset(limbs, 0xBA, sizeof(limbs));
			BigNumberUtil::unpackLE(limbs, NUM_SIZE_512BIT,
			                        expected, sizeof(expected) - posn);
			memcpy(tnum, num, sizeof(num));
			truncateNumber(tnum, sizeof(expected) - posn);
			if (memcmp(limbs, tnum, NUM_SIZE_512BIT) != 0) {
				ok = false;
			}
		}
		for (posn = 0; posn < NUM_SIZE_512BIT; ++posn) {
			memset(limbs, 0xBA, sizeof(limbs));
			BigNumberUtil::unpackLE(limbs, posn, expected, sizeof(expected));
			if (memcmp(limbs, num, posn * sizeof(limb_t)) != 0) {
				ok = false;
			}
			if (!isAllBad(limbs + posn, NUM_SIZE_512BIT - posn)) {
				ok = false;
			}
		}

		// Swap the expected byte array into big-endian order.
		for (posn = 0; posn < 32; ++posn) {
			temp = expected[posn];
			expected[posn] = expected[63 - posn];
			expected[63 - posn] = temp;
		}

		// Check packBE() and unpackBE() against the expected values.
		for (posn = 0; posn < 64; ++posn) {
			memset(bytes, 0xBA, sizeof(bytes));
			BigNumberUtil::packBE(bytes, posn, num, NUM_SIZE_512BIT);
			if (memcmp(bytes, expected + 64 - posn, posn) != 0) {
				ok = false;
			}
			if (!isAllBad(bytes + posn, sizeof(bytes) - posn)) {
				ok = false;
			}
		}
		for (posn = 0; posn < NUM_SIZE_512BIT; ++posn) {
			BigNumberUtil::packBE(bytes, sizeof(bytes), num, posn);
			if (memcmp(bytes + 64 - posn * sizeof(limb_t),
			           expected + 64 - posn * sizeof(limb_t),
			           posn * sizeof(limb_t)) != 0) {
				ok = false;
			}
			if (!isAllZero(bytes, sizeof(bytes) - posn * sizeof(limb_t))) {
				ok = false;
			}
		}
		for (posn = 0; posn < NUM_SIZE_512BIT; ++posn) {
			memset(limbs, 0xBA, sizeof(limbs));
			BigNumberUtil::unpackBE(limbs, posn, expected, sizeof(expected));
			if (memcmp(limbs, num, posn) != 0) {
				ok = false;
			}
			if (!isAllBad(limbs + posn, NUM_SIZE_512BIT - posn)) {
				ok = false;
			}
		}
		for (posn = 0; posn < 64; ++posn) {
			memset(limbs, 0xBA, sizeof(limbs));
			BigNumberUtil::unpackBE(limbs, NUM_SIZE_512BIT,
			                        expected + posn, sizeof(expected) - posn);
			memcpy(tnum, num, sizeof(num));
			truncateNumber(tnum, sizeof(expected) - posn);
			if (memcmp(limbs, tnum, NUM_SIZE_512BIT) != 0) {
				ok = false;
			}
		}
		for (posn = 0; posn < NUM_SIZE_512BIT; ++posn) {
			memset(limbs, 0xBA, sizeof(limbs));
			BigNumberUtil::unpackBE(limbs, posn, expected, sizeof(expected));
			if (memcmp(limbs, num, posn * sizeof(limb_t)) != 0) {
				ok = false;
			}
			if (!isAllBad(limbs + posn, NUM_SIZE_512BIT - posn)) {
				ok = false;
			}
		}

		// Report the results.
		if (ok) {
			Serial.println("ok");
		} else {
			Serial.println("failed");
		}
	}
}

void setup()
{
	Serial.begin(9600);

	testPackUnpack();
}

void loop()
{
}
