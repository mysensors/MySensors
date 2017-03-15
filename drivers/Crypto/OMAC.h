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

#ifndef CRYPTO_OMAC_H
#define CRYPTO_OMAC_H

#include "BlockCipher.h"

class OMAC
{
public:
	OMAC();
	~OMAC();

	BlockCipher *blockCipher() const
	{
		return _blockCipher;
	}
	void setBlockCipher(BlockCipher *cipher)
	{
		_blockCipher = cipher;
	}

	void initFirst(uint8_t omac[16]);
	void initNext(uint8_t omac[16], uint8_t tag);
	void update(uint8_t omac[16], const uint8_t *data, size_t size);
	void finalize(uint8_t omac[16]);

	void clear();

private:
	BlockCipher *_blockCipher;
	uint32_t b[4];
	uint8_t posn;
};

#endif
