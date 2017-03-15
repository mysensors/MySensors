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

#ifndef CRYPTO_SHAKE_h
#define CRYPTO_SHAKE_h

#include "XOF.h"
#include "KeccakCore.h"

class SHAKE : public XOF
{
public:
	virtual ~SHAKE();

	size_t blockSize() const;

	void reset();
	void update(const void *data, size_t len);

	void extend(uint8_t *data, size_t len);
	void encrypt(uint8_t *output, const uint8_t *input, size_t len);

	void clear();

protected:
	SHAKE(size_t capacity);

private:
	KeccakCore core;
	bool finalized;
};

class SHAKE128 : public SHAKE
{
public:
	SHAKE128() : SHAKE(256) {}
	virtual ~SHAKE128();
};

class SHAKE256 : public SHAKE
{
public:
	SHAKE256() : SHAKE(512) {}
	virtual ~SHAKE256();
};

#endif
