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

#ifndef CRYPTO_EAX_h
#define CRYPTO_EAX_h

#include "AuthenticatedCipher.h"
#include "BlockCipher.h"
#include "OMAC.h"

class EAXCommon : public AuthenticatedCipher
{
public:
	virtual ~EAXCommon();

	size_t keySize() const;
	size_t ivSize() const;
	size_t tagSize() const;

	bool setKey(const uint8_t *key, size_t len);
	bool setIV(const uint8_t *iv, size_t len);

	void encrypt(uint8_t *output, const uint8_t *input, size_t len);
	void decrypt(uint8_t *output, const uint8_t *input, size_t len);

	void addAuthData(const void *data, size_t len);

	void computeTag(void *tag, size_t len);
	bool checkTag(const void *tag, size_t len);

	void clear();

protected:
	EAXCommon();
	void setBlockCipher(BlockCipher *cipher)
	{
		omac.setBlockCipher(cipher);
	}

private:
	struct {
		uint8_t counter[16];
		uint8_t stream[16];
		uint8_t tag[16];
		uint8_t hash[16];
		uint8_t encPosn;
		uint8_t authMode;
	} state;
	OMAC omac;

	void closeAuthData();
	void encryptCTR(uint8_t *output, const uint8_t *input, size_t len);
	void closeTag();
};

template <typename T>
class EAX : public EAXCommon
{
public:
	EAX()
	{
		setBlockCipher(&cipher);
	}

private:
	T cipher;
};

#endif
