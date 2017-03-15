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

#ifndef CRYPTO_POLY1305_h
#define CRYPTO_POLY1305_h

#include "BigNumberUtil.h"
#include <stddef.h>

class Poly1305
{
public:
	Poly1305();
	~Poly1305();

	void reset(const void *key);
	void update(const void *data, size_t len);
	void finalize(const void *nonce, void *token, size_t len);

	void pad();

	void clear();

private:
	struct {
		limb_t h[(16 / sizeof(limb_t)) + 1];
		limb_t c[(16 / sizeof(limb_t)) + 1];
		limb_t r[(16 / sizeof(limb_t))];
		uint8_t chunkSize;
	} state;

	void processChunk();
};

#endif
