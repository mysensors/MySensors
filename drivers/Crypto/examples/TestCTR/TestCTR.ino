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
This example runs tests on the CTR implementation to verify correct behaviour.
*/

#include <Crypto.h>
#include <AES.h>
#include <CTR.h>
#include <string.h>

#define MAX_PLAINTEXT_SIZE  36
#define MAX_CIPHERTEXT_SIZE 36

struct TestVector {
	const char *name;
	byte key[16];
	byte plaintext[MAX_PLAINTEXT_SIZE];
	byte ciphertext[MAX_CIPHERTEXT_SIZE];
	byte iv[16];
	size_t size;
};

// Test vectors for AES-128 in CTR mode from RFC 3686.
static TestVector const testVectorAES128CTR1 = {
	.name        = "AES-128-CTR #1",
	.key         = {
		0xAE, 0x68, 0x52, 0xF8, 0x12, 0x10, 0x67, 0xCC,
		0x4B, 0xF7, 0xA5, 0x76, 0x55, 0x77, 0xF3, 0x9E
	},
	.plaintext   = {
		0x53, 0x69, 0x6E, 0x67, 0x6C, 0x65, 0x20, 0x62,
		0x6C, 0x6F, 0x63, 0x6B, 0x20, 0x6D, 0x73, 0x67
	},
	.ciphertext  = {
		0xE4, 0x09, 0x5D, 0x4F, 0xB7, 0xA7, 0xB3, 0x79,
		0x2D, 0x61, 0x75, 0xA3, 0x26, 0x13, 0x11, 0xB8
	},
	.iv          = {
		0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	},
	.size        = 16
};
static TestVector const testVectorAES128CTR2 = {
	.name        = "AES-128-CTR #2",
	.key         = {
		0x7E, 0x24, 0x06, 0x78, 0x17, 0xFA, 0xE0, 0xD7,
		0x43, 0xD6, 0xCE, 0x1F, 0x32, 0x53, 0x91, 0x63
	},
	.plaintext   = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
	},
	.ciphertext  = {
		0x51, 0x04, 0xA1, 0x06, 0x16, 0x8A, 0x72, 0xD9,
		0x79, 0x0D, 0x41, 0xEE, 0x8E, 0xDA, 0xD3, 0x88,
		0xEB, 0x2E, 0x1E, 0xFC, 0x46, 0xDA, 0x57, 0xC8,
		0xFC, 0xE6, 0x30, 0xDF, 0x91, 0x41, 0xBE, 0x28
	},
	.iv          = {
		0x00, 0x6C, 0xB6, 0xDB, 0xC0, 0x54, 0x3B, 0x59,
		0xDA, 0x48, 0xD9, 0x0B, 0x00, 0x00, 0x00, 0x01
	},
	.size        = 32
};
static TestVector const testVectorAES128CTR3 = {
	.name        = "AES-128-CTR #3",
	.key         = {
		0x76, 0x91, 0xBE, 0x03, 0x5E, 0x50, 0x20, 0xA8,
		0xAC, 0x6E, 0x61, 0x85, 0x29, 0xF9, 0xA0, 0xDC
	},
	.plaintext   = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		0x20, 0x21, 0x22, 0x23
	},
	.ciphertext  = {
		0xC1, 0xCF, 0x48, 0xA8, 0x9F, 0x2F, 0xFD, 0xD9,
		0xCF, 0x46, 0x52, 0xE9, 0xEF, 0xDB, 0x72, 0xD7,
		0x45, 0x40, 0xA4, 0x2B, 0xDE, 0x6D, 0x78, 0x36,
		0xD5, 0x9A, 0x5C, 0xEA, 0xAE, 0xF3, 0x10, 0x53,
		0x25, 0xB2, 0x07, 0x2F
	},
	.iv          = {
		0x00, 0xE0, 0x01, 0x7B, 0x27, 0x77, 0x7F, 0x3F,
		0x4A, 0x17, 0x86, 0xF0, 0x00, 0x00, 0x00, 0x01
	},
	.size        = 36
};

CTR<AES128> ctraes128;

byte buffer[128];

bool testCipher_N(Cipher *cipher, const struct TestVector *test, size_t inc)
{
	byte output[MAX_CIPHERTEXT_SIZE];
	size_t posn, len;

	cipher->clear();
	if (!cipher->setKey(test->key, cipher->keySize())) {
		Serial.print("setKey ");
		return false;
	}
	if (!cipher->setIV(test->iv, cipher->ivSize())) {
		Serial.print("setIV ");
		return false;
	}

	memset(output, 0xBA, sizeof(output));

	for (posn = 0; posn < test->size; posn += inc) {
		len = test->size - posn;
		if (len > inc) {
			len = inc;
		}
		cipher->encrypt(output + posn, test->plaintext + posn, len);
	}

	if (memcmp(output, test->ciphertext, test->size) != 0) {
		Serial.print(output[0], HEX);
		Serial.print("->");
		Serial.print(test->ciphertext[0], HEX);
		return false;
	}

	cipher->setKey(test->key, cipher->keySize());
	cipher->setIV(test->iv, cipher->ivSize());

	for (posn = 0; posn < test->size; posn += inc) {
		len = test->size - posn;
		if (len > inc) {
			len = inc;
		}
		cipher->decrypt(output + posn, test->ciphertext + posn, len);
	}

	if (memcmp(output, test->plaintext, test->size) != 0) {
		return false;
	}

	return true;
}

void testCipher(Cipher *cipher, const struct TestVector *test)
{
	bool ok;

	Serial.print(test->name);
	Serial.print(" ... ");

	ok  = testCipher_N(cipher, test, test->size);
	ok &= testCipher_N(cipher, test, 1);
	ok &= testCipher_N(cipher, test, 2);
	ok &= testCipher_N(cipher, test, 5);
	ok &= testCipher_N(cipher, test, 8);
	ok &= testCipher_N(cipher, test, 13);
	ok &= testCipher_N(cipher, test, 16);

	if (ok) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
	}
}

void perfCipherEncrypt(const char *name, Cipher *cipher, const struct TestVector *test)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print(name);
	Serial.print(" ... ");

	cipher->setKey(test->key, cipher->keySize());
	cipher->setIV(test->iv, cipher->ivSize());
	start = micros();
	for (count = 0; count < 500; ++count) {
		cipher->encrypt(buffer, buffer, sizeof(buffer));
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (sizeof(buffer) * 500.0));
	Serial.print("us per byte, ");
	Serial.print((sizeof(buffer) * 500.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");
}

void perfCipherDecrypt(const char *name, Cipher *cipher, const struct TestVector *test)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print(name);
	Serial.print(" ... ");

	cipher->setKey(test->key, cipher->keySize());
	cipher->setIV(test->iv, cipher->ivSize());
	start = micros();
	for (count = 0; count < 500; ++count) {
		cipher->decrypt(buffer, buffer, sizeof(buffer));
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (sizeof(buffer) * 500.0));
	Serial.print("us per byte, ");
	Serial.print((sizeof(buffer) * 500.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");
}

void setup()
{
	Serial.begin(9600);

	Serial.println();

	Serial.println("Test Vectors:");
	testCipher(&ctraes128, &testVectorAES128CTR1);
	testCipher(&ctraes128, &testVectorAES128CTR2);
	testCipher(&ctraes128, &testVectorAES128CTR3);

	Serial.println();

	Serial.println("Performance Tests:");
	perfCipherEncrypt("AES-128-CTR Encrypt", &ctraes128, &testVectorAES128CTR1);
	perfCipherDecrypt("AES-128-CTR Decrypt", &ctraes128, &testVectorAES128CTR1);
}

void loop()
{
}
