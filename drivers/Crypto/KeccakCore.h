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

#ifndef CRYPTO_KECCAKCORE_H
#define CRYPTO_KECCAKCORE_H

#include <inttypes.h>
#include <stddef.h>

class KeccakCore
{
public:
	KeccakCore();
	~KeccakCore();

	size_t capacity() const;
	void setCapacity(size_t capacity);

	size_t blockSize() const
	{
		return _blockSize;
	}

	void reset();

	void update(const void *data, size_t size);
	void pad(uint8_t tag);

	void extract(void *data, size_t size);
	void encrypt(void *output, const void *input, size_t size);

	void clear();

	void setHMACKey(const void *key, size_t len, uint8_t pad, size_t hashSize);

private:
	struct {
		uint64_t A[5][5];
		uint8_t inputSize;
		uint8_t outputSize;
	} state;
	uint8_t _blockSize;

	void keccakp();
};

#endif
