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

#ifndef CRYPTO_P521_h
#define CRYPTO_P521_h

#include "BigNumberUtil.h"

class Hash;

class P521
{
public:

	static bool eval(uint8_t result[132], const uint8_t f[66], const uint8_t point[132]);

	static void dh1(uint8_t k[132], uint8_t f[66]);
	static bool dh2(const uint8_t k[132], uint8_t f[66]);

	static void sign(uint8_t signature[132], const uint8_t privateKey[66],
	                 const void *message, size_t len, Hash *hash = 0);
	static bool verify(const uint8_t signature[132],
	                   const uint8_t publicKey[132],
	                   const void *message, size_t len, Hash *hash = 0);

	static void generatePrivateKey(uint8_t privateKey[66]);
	static void derivePublicKey(uint8_t publicKey[132], const uint8_t privateKey[66]);

	static bool isValidPrivateKey(const uint8_t privateKey[66]);
	static bool isValidPublicKey(const uint8_t publicKey[132]);

	static bool isValidCurvePoint(const uint8_t point[132])
	{
		return isValidPublicKey(point);
	}

#if defined(TEST_P521_FIELD_OPS)
public:
#else
private:
#endif
	static void evaluate(limb_t *x, limb_t *y, const uint8_t f[66]);

	static void addAffine(limb_t *x1, limb_t *y1,
	                      const limb_t *x2, const limb_t *y2);

	static bool validate(const limb_t *x, const limb_t *y);
	static bool inRange(const limb_t *x);

	static void reduce(limb_t *result, const limb_t *x);
	static void reduceQuick(limb_t *x);

	static void mulNoReduce(limb_t *result, const limb_t *x, const limb_t *y);

	static void mul(limb_t *result, const limb_t *x, const limb_t *y);
	static void square(limb_t *result, const limb_t *x)
	{
		mul(result, x, x);
	}

	static void mulLiteral(limb_t *result, const limb_t *x, limb_t y);

	static void add(limb_t *result, const limb_t *x, const limb_t *y);
	static void sub(limb_t *result, const limb_t *x, const limb_t *y);

	static void dblPoint(limb_t *xout, limb_t *yout, limb_t *zout,
	                     const limb_t *xin, const limb_t *yin,
	                     const limb_t *zin);
	static void addPoint(limb_t *xout, limb_t *yout, limb_t *zout,
	                     const limb_t *x1, const limb_t *y1,
	                     const limb_t *z1, const limb_t *x2,
	                     const limb_t *y2);

	static void cmove(limb_t select, limb_t *x, const limb_t *y);
	static void cmove1(limb_t select, limb_t *x);

	static void recip(limb_t *result, const limb_t *x);

	static void reduceQ(limb_t *result, const limb_t *r);
	static void mulQ(limb_t *result, const limb_t *x, const limb_t *y);
	static void recipQ(limb_t *result, const limb_t *x);

	static void generateK(uint8_t k[66], const uint8_t hm[66],
	                      const uint8_t x[66], Hash *hash, uint64_t count);
	static void generateK(uint8_t k[66], const uint8_t hm[66],
	                      const uint8_t x[66], uint64_t count);

	// Constructor and destructor are private - cannot instantiate this class.
	P521() {}
	~P521() {}
};

#endif
