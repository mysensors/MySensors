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

#ifndef CRYPTO_BLAKE2B_H
#define CRYPTO_BLAKE2B_H

#include "Hash.h"

class BLAKE2b : public Hash
{
public:
	BLAKE2b();
	virtual ~BLAKE2b();

	size_t hashSize() const;
	size_t blockSize() const;

	void reset();
	void reset(uint8_t outputLength);
	void reset(const void *key, size_t keyLen, uint8_t outputLength = 64);

	void update(const void *data, size_t len);
	void finalize(void *hash, size_t len);

	void clear();

	void resetHMAC(const void *key, size_t keyLen);
	void finalizeHMAC(const void *key, size_t keyLen, void *hash, size_t hashLen);

private:
	struct {
		uint64_t h[8];
		uint64_t m[16];
		uint64_t lengthLow;
		uint64_t lengthHigh;
		uint8_t chunkSize;
	} state;

	void processChunk(uint64_t f0);
};

#endif
