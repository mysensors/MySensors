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
This example runs tests on the Curve25519 field mathematics independent
of the full curve operation itself.
*/

// Enable access to the internals of Curve25519 to test the raw field ops.
#define TEST_CURVE25519_FIELD_OPS 1

#include <Crypto.h>
#include <Curve25519.h>
#include <utility/ProgMemUtil.h>
#include <string.h>

// Copy some definitions from the Curve25519 class for convenience.
#define NUM_LIMBS   (32 / sizeof(limb_t))
#define LIMB_BITS   (8 * sizeof(limb_t))
#define INVERSE_LIMB (~((limb_t)0))

// For simpleMod() below we need a type that is 4 times the size of limb_t.
#if BIGNUMBER_LIMB_8BIT
#define qlimb_t uint32_t
#elif BIGNUMBER_LIMB_16BIT
#define qlimb_t uint64_t
#else
#define BIGNUMBER_NO_QLIMB 1
#endif

limb_t arg1[NUM_LIMBS];
limb_t arg2[NUM_LIMBS];
limb_t result[NUM_LIMBS];
limb_t result2[NUM_LIMBS * 2 + 1];
limb_t temp[NUM_LIMBS];

// Convert a decimal string in program memory into a number.
void fromString(limb_t *x, uint8_t size, const char *str)
{
	uint8_t ch, posn;
	memset(x, 0, sizeof(limb_t) * size);
	while ((ch = pgm_read_byte((uint8_t *)str)) != '\0') {
		if (ch >= '0' && ch <= '9') {
			// Quick and simple method to multiply by 10 and add the new digit.
			dlimb_t carry = ch - '0';
			for (posn = 0; posn < size; ++posn) {
				carry += ((dlimb_t)x[posn]) * 10U;
				x[posn] = (limb_t)carry;
				carry >>= LIMB_BITS;
			}
		}
		++str;
	}
}

// Compare two numbers of NUM_LIMBS in length.  Returns -1, 0, or 1.
int compare(const limb_t *x, const limb_t *y)
{
	for (uint8_t posn = NUM_LIMBS; posn > 0; --posn) {
		limb_t a = x[posn - 1];
		limb_t b = y[posn - 1];
		if (a < b) {
			return -1;
		} else if (a > b) {
			return 1;
		}
	}
	return 0;
}

// Compare two numbers where one is a decimal string.  Returns -1, 0, or 1.
int compare(const limb_t *x, const char *y)
{
	limb_t val[NUM_LIMBS];
	fromString(val, NUM_LIMBS, y);
	return compare(x, val);
}

void printNumber(const char *name, const limb_t *x)
{
	static const char hexchars[] = "0123456789ABCDEF";
	Serial.print(name);
	Serial.print(" = ");
	for (uint8_t posn = NUM_LIMBS; posn > 0; --posn) {
		for (uint8_t bit = LIMB_BITS; bit > 0; ) {
			bit -= 4;
			Serial.print(hexchars[(x[posn - 1] >> bit) & 0x0F]);
		}
		Serial.print(' ');
	}
	Serial.println();
}

// Standard numbers that are useful in field operation tests.
char const num_0[] PROGMEM = "0";
char const num_1[] PROGMEM = "1";
char const num_2[] PROGMEM = "2";
char const num_4[] PROGMEM = "4";
char const num_5[] PROGMEM = "5";
char const num_128[] PROGMEM = "128";
char const num_256[] PROGMEM = "256";
char const num_2_64_m7[] PROGMEM = "18446744073709551609"; // 2^64 - 7
char const num_2_129_m5[] PROGMEM = "680564733841876926926749214863536422907"; // 2^129 - 5
char const num_pi[] PROGMEM =
    "31415926535897932384626433832795028841971693993751058209749445923078164062862"; // 77 digits of pi
char const num_2_255_m253[] PROGMEM =
    "57896044618658097711785492504343953926634992332820282019728792003956564819715"; // 2^255 - 253
char const num_2_255_m20[] PROGMEM =
    "57896044618658097711785492504343953926634992332820282019728792003956564819948"; // 2^255 - 20
char const num_2_255_m19[] PROGMEM =
    "57896044618658097711785492504343953926634992332820282019728792003956564819949"; // 2^255 - 19
char const num_2_255_m19_x2[] PROGMEM =
    "115792089237316195423570985008687907853269984665640564039457584007913129639898"; // (2^255 - 19) * 2
char const num_a24[] PROGMEM = "121665";

// Table of useful numbers less than 2^255 - 19.
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
	num_pi,
	num_2_255_m253,
	num_2_255_m20,
	0
};
#define numbers_count   ((sizeof(numbers) / sizeof(numbers[0])) - 1)

#define foreach_number(var) \
	const char *var = numbers[0]; \
	for (unsigned index##var = 0; index##var < numbers_count; \
	        ++index##var, var = numbers[index##var])

void printProgMem(const char *str)
{
	uint8_t ch;
	while ((ch = pgm_read_byte((uint8_t *)str)) != '\0') {
		Serial.print((char)ch);
		++str;
	}
}

// Simple implementation of modular addition to cross-check the library.
void simpleAdd(limb_t *result, const limb_t *x, const limb_t *y)
{
	uint8_t posn;
	dlimb_t carry = 0;
	for (posn = 0; posn < NUM_LIMBS; ++posn) {
		carry += x[posn];
		carry += y[posn];
		result[posn] = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	if (compare(result, num_2_255_m19) >= 0) {
		// Subtract 2^255 - 19 to get the final result.
		// Same as add 19 and then subtract 2^255.
		carry = 19;
		for (posn = 0; posn < NUM_LIMBS; ++posn) {
			carry += result[posn];
			result[posn] = (limb_t)carry;
			carry >>= LIMB_BITS;
		}
		result[NUM_LIMBS - 1] -= ((limb_t)1) << (LIMB_BITS - 1);
	}
}

// Simple implementation of subtraction to cross-check the library.
// Note: this does not reduce the result modulo 2^255 - 19 and we
// assume that x is greater than or equal to y.
void simpleSub(limb_t *result, const limb_t *x, const limb_t *y)
{
	uint8_t posn;
	dlimb_t borrow = 0;
	for (posn = 0; posn < NUM_LIMBS; ++posn) {
		borrow = ((dlimb_t)x[posn]) - y[posn] - borrow;
		result[posn] = (limb_t)borrow;
		borrow = (borrow >> LIMB_BITS) != 0;
	}
}

// Simple implementation of multiplication to cross-check the library.
// Note: this does not reduce the result modulo 2^255 - 19.
// The "result" buffer must contain at least NUM_LIMBS * 2 limbs.
void simpleMul(limb_t *result, const limb_t *x, const limb_t *y)
{
	memset(result, 0, NUM_LIMBS * 2 * sizeof(limb_t));
	for (uint8_t i = 0; i < NUM_LIMBS; ++i) {
		for (uint8_t j = 0; j < NUM_LIMBS; ++j) {
			uint8_t n = i + j;
			dlimb_t carry =
			    ((dlimb_t)x[i]) * y[j] + result[n];
			result[n] = (limb_t)carry;
			carry >>= LIMB_BITS;
			++n;
			while (carry != 0 && n < (NUM_LIMBS * 2)) {
				carry += result[n];
				result[n] = (limb_t)carry;
				carry >>= LIMB_BITS;
				++n;
			}
		}
	}
}

#if defined(BIGNUMBER_NO_QLIMB)

// Quick check to correct the estimate on a quotient word.
static inline limb_t correctEstimate
(limb_t q, limb_t y1, limb_t y2, dlimb_t x01, limb_t x2)
{
	// Algorithm D from section 4.3.1 of "The Art Of Computer Programming",
	// D. Knuth, Volume 2, "Seminumerical Algorithms", Second Edition, 1981.
	//
	// We want to check if (y2 * q) > ((x01 - y1 * q) * b + x2) where
	// b is (1 << LIMB_BITS).  If it is, then q must be reduced by 1.
	//
	// One wrinkle that isn't obvious from Knuth's description is that it
	// is possible for (x01 - y1 * q) >= b, especially in the case where
	// x0 = y1 and q = b - 1.  This will cause an overflow of the intermediate
	// double-word result ((x01 - y1 * q) * b).
	//
	// In assembly language, we could use the carry flag to detect when
	// (x01 - y1 * q) * b overflows, but we can't access the carry flag
	// in C++.  So we have to account for the carry in a different way here.

	// Calculate the remainder using the estimated quotient.
	dlimb_t r = x01 - ((dlimb_t)y1) * q;

	// If there will be a double-word carry when we calculate (r * b),
	// then (y2 * q) is obviously going to be less than (r * b), so we
	// can stop here.  The estimated quotient is correct.
	if (r & (((dlimb_t)INVERSE_LIMB) << LIMB_BITS)) {
		return q;
	}

	// Bail out if (y2 * q) <= (r * b + x2).  The estimate is correct.
	dlimb_t y2q = ((dlimb_t)y2) * q;
	if (y2q <= ((r << LIMB_BITS) + x2)) {
		return q;
	}

	// Correct for the estimated quotient being off by 1.
	--q;

	// Now repeat the check to correct for q values that are off by 2.
	r += y1;    // r' = (x01 - y1 * (q - 1)) = (x01 - y1 * q + y2) = r + y1
	if (r & (((dlimb_t)INVERSE_LIMB) << LIMB_BITS)) {
		return q;
	}
	// y2q' = (y2 * (q - 1)) = (y2 * q - y2) = y2q - y2
	if ((y2q - y2) <= ((r << LIMB_BITS) + x2)) {
		return q;
	}

	// Perform the final correction for q values that are off by 2.
	return q - 1;
}

#endif

// Simple implementation of modular division to cross-check the library.
// Calling this "simple" is a bit of a misnomer.  It is a full implementation
// of Algorithm D from section 4.3.1 of "The Art Of Computer Programming",
// D. Knuth, Volume 2, "Seminumerical Algorithms", Second Edition, 1981.
// This is quite slow on embedded platforms, but it should be correct.
// Note: "x" is assumed to be (NUM_LIMBS * 2 + 1) limbs in size because
// we need a limb for the extra leading zero word added by step D1.
void simpleMod(limb_t *x)
{
	limb_t divisor[NUM_LIMBS];
	uint8_t j, k;

	// Step D1. Normalize.
	// The divisor (2^255 - 19) and "x" need to be shifted left until
	// the top-most bit of the divisor is 1.  Since we know that the
	// next-to-top-most bit of (2^255 - 19) is already 1 and the top-most
	// bit of "x" is zero, shifting everything into place is pretty easy.
	fromString(divisor, NUM_LIMBS, num_2_255_m19_x2);
	for (j = (NUM_LIMBS * 2); j > 1; --j) {
		x[j - 1] = (x[j - 1] << 1) | (x[j - 2] >> (LIMB_BITS - 1));
	}
	x[0] <<= 1;
	x[NUM_LIMBS * 2] = 0;   // Extra leading word.

	// Step D2/D7. Loop on j
	for (j = 0; j <= NUM_LIMBS; ++j) {
		// Step D3. Calculate an estimate of the top-most quotient word.
		limb_t *u = x + NUM_LIMBS * 2 - 2 - j;
		limb_t *v = divisor + NUM_LIMBS - 2;
		limb_t q;
		dlimb_t uword = ((((dlimb_t)u[2]) << LIMB_BITS) + u[1]);
		if (u[2] == v[1]) {
			q = ~((limb_t)0);
		} else {
			q = (limb_t)(uword / v[1]);
		}

		// Step D3, part 2.  Correct the estimate downwards by 1 or 2.
		// One subtlety of Knuth's algorithm is that it looks like the test
		// is working with double-word quantities but it is actually using
		// double-word plus a carry bit.  So we need to use qlimb_t for this.
#if !defined(BIGNUMBER_NO_QLIMB)
		qlimb_t test = ((((qlimb_t)uword) - ((dlimb_t)q) * v[1]) << LIMB_BITS) + u[0];
		if ((((dlimb_t)q) * v[0]) > test) {
			--q;
			test = ((((qlimb_t)uword) - ((dlimb_t)q) * v[1]) << LIMB_BITS) + u[0];
			if ((((dlimb_t)q) * v[0]) > test) {
				--q;
			}
		}
#else
		// 32-bit platform - we don't have a 128-bit numeric type so we have
		// to calculate the estimate in another way to preserve the carry bit.
		q = correctEstimate(q, v[0], v[1], uword, u[0]);
#endif

		// Step D4. Multiply and subtract.
		u = x + (NUM_LIMBS - j);
		v = divisor;
		dlimb_t carry = 0;
		dlimb_t borrow = 0;
		for (k = 0; k < NUM_LIMBS; ++k) {
			carry += ((dlimb_t)v[k]) * q;
			borrow = ((dlimb_t)u[k]) - ((limb_t)carry) - borrow;
			u[k] = (dlimb_t)borrow;
			carry >>= LIMB_BITS;
			borrow = ((borrow >> LIMB_BITS) != 0);
		}
		borrow = ((dlimb_t)u[k]) - ((limb_t)carry) - borrow;
		u[k] = (dlimb_t)borrow;

		// Step D5. Test remainder.  Nothing further to do if no borrow.
		if ((borrow >> LIMB_BITS) == 0) {
			continue;
		}

		// Step D6. Borrow occurred: add back.
		carry = 0;
		for (k = 0; k < NUM_LIMBS; ++k) {
			carry += u[k];
			carry += v[k];
			u[k] = (limb_t)carry;
			carry >>= LIMB_BITS;
		}
		u[k] += (limb_t)carry;
	}

	// Step D8. Unnormalize.
	// Shift the remainder right by 1 bit to undo the earlier left shift.
	for (j = 0; j < (NUM_LIMBS - 1); ++j) {
		x[j] = (x[j] >> 1) | (x[j + 1] << (LIMB_BITS - 1));
	}
	x[NUM_LIMBS - 1] >>= 1;
}

void testAdd(const char *x, const char *y)
{
	printProgMem(x);
	Serial.print(" + ");
	printProgMem(y);
	Serial.print(": ");
	Serial.flush();

	fromString(arg1, NUM_LIMBS, x);
	fromString(arg2, NUM_LIMBS, y);
	Curve25519::add(result, arg1, arg2);

	simpleAdd(result2, arg1, arg2);

	if (compare(result, result2) == 0) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", result);
		printNumber("expected", result2);
	}
}

void testAdd()
{
	Serial.println("Addition:");
	foreach_number (x) {
		foreach_number (y) {
			testAdd(x, y);
		}
	}
	Serial.println();
}

void testSub(const char *x, const char *y)
{
	printProgMem(x);
	Serial.print(" - ");
	printProgMem(y);
	Serial.print(": ");
	Serial.flush();

	fromString(arg1, NUM_LIMBS, x);
	fromString(arg2, NUM_LIMBS, y);
	Curve25519::sub(result, arg1, arg2);

	if (compare(arg1, arg2) >= 0) {
		// First argument is larger than the second.
		simpleSub(result2, arg1, arg2);
	} else {
		// First argument is smaller than the second.
		// Compute arg1 + (2^255 - 19 - arg2).
		fromString(temp, NUM_LIMBS, num_2_255_m19);
		simpleSub(result2, temp, arg2);
		simpleAdd(result2, arg1, result2);
	}

	if (compare(result, result2) == 0) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", result);
		printNumber("expected", result2);
	}
}

void testSub()
{
	Serial.println("Subtraction:");
	foreach_number (x) {
		foreach_number (y) {
			testSub(x, y);
		}
	}
	Serial.println();
}

void testMul(const char *x, const char *y)
{
	printProgMem(x);
	Serial.print(" * ");
	printProgMem(y);
	Serial.print(": ");
	Serial.flush();

	fromString(arg1, NUM_LIMBS, x);
	fromString(arg2, NUM_LIMBS, y);

	if (compare(arg1, arg2) != 0) {
		Curve25519::mul(result, arg1, arg2);
	} else {
		Curve25519::square(result, arg1);
	}

	simpleMul(result2, arg1, arg2);
	simpleMod(result2);

	if (compare(result, result2) == 0) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", result);
		printNumber("expected", result2);
	}
}

void testMul()
{
	Serial.println("Multiplication:");
	foreach_number (x) {
		foreach_number (y) {
			testMul(x, y);
		}
	}
	Serial.println();
}

void testMulA24(const char *x)
{
	printProgMem(x);
	Serial.print(" * ");
	printProgMem(num_a24);
	Serial.print(": ");
	Serial.flush();

	fromString(arg1, NUM_LIMBS, x);
	fromString(arg2, NUM_LIMBS, num_a24);
	Curve25519::mulA24(result, arg1);

	simpleMul(result2, arg1, arg2);
	simpleMod(result2);

	if (compare(result, result2) == 0) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", result);
		printNumber("expected", result2);
	}
}

void testMulA24()
{
	Serial.println("Multiplication by a24:");
	foreach_number (x) {
		testMulA24(x);
	}
	Serial.println();
}

void testSwap(const char *x, const char *y, uint8_t select)
{
	printProgMem(x);
	Serial.print(" <-> ");
	printProgMem(y);
	Serial.print(": ");
	Serial.flush();

	fromString(arg1, NUM_LIMBS, x);
	fromString(arg2, NUM_LIMBS, y);

	memcpy(result, arg1, NUM_LIMBS * sizeof(limb_t));
	memcpy(result2, arg2, NUM_LIMBS * sizeof(limb_t));

	// Swap the values using the selection bit.
	Curve25519::cswap(select, result, result2);
	bool ok = compare(result, arg2) == 0 && compare(result2, arg1) == 0;

	// Don't swap the values back yet.
	Curve25519::cswap(0, result, result2);
	if (ok) {
		ok = compare(result, arg2) == 0 && compare(result2, arg1) == 0;
	}

	// Swap the values back.
	Curve25519::cswap(select, result, result2);
	if (ok) {
		ok = compare(result, arg1) == 0 && compare(result2, arg2) == 0;
	}

	// No swap.
	Curve25519::cswap(0, result, result2);
	if (ok) {
		ok = compare(result, arg1) == 0 && compare(result2, arg2) == 0;
	}

	if (ok) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
	}
}

void testSwap()
{
	Serial.println("Swap:");
	uint8_t bit = 0;
	foreach_number (x) {
		foreach_number (y) {
			testSwap(x, y, ((uint8_t)1) << bit);
			bit = (bit + 1) % 8;
		}
	}
	Serial.println();
}

void testRecip(const char *x)
{
	printProgMem(x);
	Serial.print("^-1");
	Serial.print(": ");
	Serial.flush();

	fromString(arg1, NUM_LIMBS, x);
	Curve25519::recip(result, arg1);

	bool ok;
	if (compare(arg1, num_0) == 0) {
		// 0^-1 = 0
		ok = (compare(result, num_0) == 0);
	} else {
		// Multiply the result with arg1 - we expect 1 as the result.
		Curve25519::mul(result2, result, arg1);
		ok = (compare(result2, num_1) == 0);
	}

	if (ok) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
		printNumber("actual", result);
	}
}

void testRecip()
{
	Serial.println("Reciprocal:");
	foreach_number (x) {
		testRecip(x);
	}
	Serial.println();
}

void testSqrt(const char *x)
{
	Serial.print("sqrt(");
	printProgMem(x);
	Serial.print("^2): ");
	Serial.flush();

	fromString(arg1, NUM_LIMBS, x);
	Curve25519::square(arg2, arg1);
	bool ok = Curve25519::sqrt(result, arg2);

	if (ok) {
		ok = (compare(result, arg1) == 0);
		if (!ok) {
			// Check the negation of arg1 as well because we could
			// have ended up with the inverse of the original value.
			memset(temp, 0, sizeof(temp));
			Curve25519::sub(temp, temp, arg1);
			ok = (compare(result, temp) == 0);
		}
	} else {
		Serial.println("no sqrt ... ");
	}

	if (ok) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
		printNumber("actual", result);
		printNumber("expected", arg1);
	}
}

void testNoSqrt(const char *x)
{
	Serial.print("no sqrt(");
	printProgMem(x);
	Serial.print("): ");
	Serial.flush();

	fromString(arg1, NUM_LIMBS, x);
	bool ok = !Curve25519::sqrt(result, arg1);

	if (ok) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
		printNumber("actual", result);
	}
}

void testSqrt()
{
	Serial.println("Square root:");
	foreach_number (x) {
		testSqrt(x);
	}
	testNoSqrt(num_128);
	testNoSqrt(num_pi);
	Serial.println();
}

void setup()
{
	Serial.begin(9600);

	testAdd();
	testSub();
	testMul();
	testMulA24();
	testSwap();
	testRecip();
	testSqrt();
}

void loop()
{
}
