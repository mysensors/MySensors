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
This example runs tests on the EAX implementation to verify correct behaviour.
*/

#include <Crypto.h>
#include <EAX.h>
#include <AES.h>
#include <Speck.h>
#include <SpeckTiny.h>
#include <string.h>
#include <avr/pgmspace.h>

#define MAX_PLAINTEXT_LEN 64

struct TestVector {
	const char *name;
	uint8_t key[16];
	uint8_t plaintext[MAX_PLAINTEXT_LEN];
	uint8_t ciphertext[MAX_PLAINTEXT_LEN];
	uint8_t authdata[20];
	uint8_t iv[16];
	uint8_t tag[16];
	size_t authsize;
	size_t datasize;
	size_t tagsize;
	size_t ivsize;
};

// Test vectors for AES in EAX mode from Appendix G of:
// http://www.cs.ucdavis.edu/~rogaway/papers/eax.pdf
static TestVector const testVectorEAX1 PROGMEM = {
	.name        = "EAX #1",
	.key         = {
		0x23, 0x39, 0x52, 0xDE, 0xE4, 0xD5, 0xED, 0x5F,
		0x9B, 0x9C, 0x6D, 0x6F, 0xF8, 0x0F, 0xF4, 0x78
	},
	.plaintext   = {0x00},
	.ciphertext  = {0x00},
	.authdata    = {0x6B, 0xFB, 0x91, 0x4F, 0xD0, 0x7E, 0xAE, 0x6B},
	.iv          = {
		0x62, 0xEC, 0x67, 0xF9, 0xC3, 0xA4, 0xA4, 0x07,
		0xFC, 0xB2, 0xA8, 0xC4, 0x90, 0x31, 0xA8, 0xB3
	},
	.tag         = {
		0xE0, 0x37, 0x83, 0x0E, 0x83, 0x89, 0xF2, 0x7B,
		0x02, 0x5A, 0x2D, 0x65, 0x27, 0xE7, 0x9D, 0x01
	},
	.authsize    = 8,
	.datasize    = 0,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX2 PROGMEM = {
	.name        = "EAX #2",
	.key         = {
		0x91, 0x94, 0x5D, 0x3F, 0x4D, 0xCB, 0xEE, 0x0B,
		0xF4, 0x5E, 0xF5, 0x22, 0x55, 0xF0, 0x95, 0xA4
	},
	.plaintext   = {0xF7, 0xFB},
	.ciphertext  = {0x19, 0xDD},
	.authdata    = {0xFA, 0x3B, 0xFD, 0x48, 0x06, 0xEB, 0x53, 0xFA},
	.iv          = {
		0xBE, 0xCA, 0xF0, 0x43, 0xB0, 0xA2, 0x3D, 0x84,
		0x31, 0x94, 0xBA, 0x97, 0x2C, 0x66, 0xDE, 0xBD
	},
	.tag         = {
		0x5C, 0x4C, 0x93, 0x31, 0x04, 0x9D, 0x0B, 0xDA,
		0xB0, 0x27, 0x74, 0x08, 0xF6, 0x79, 0x67, 0xE5
	},
	.authsize    = 8,
	.datasize    = 2,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX3 PROGMEM = {
	.name        = "EAX #3",
	.key         = {
		0x01, 0xF7, 0x4A, 0xD6, 0x40, 0x77, 0xF2, 0xE7,
		0x04, 0xC0, 0xF6, 0x0A, 0xDA, 0x3D, 0xD5, 0x23
	},
	.plaintext   = {0x1A, 0x47, 0xCB, 0x49, 0x33},
	.ciphertext  = {0xD8, 0x51, 0xD5, 0xBA, 0xE0},
	.authdata    = {0x23, 0x4A, 0x34, 0x63, 0xC1, 0x26, 0x4A, 0xC6},
	.iv          = {
		0x70, 0xC3, 0xDB, 0x4F, 0x0D, 0x26, 0x36, 0x84,
		0x00, 0xA1, 0x0E, 0xD0, 0x5D, 0x2B, 0xFF, 0x5E
	},
	.tag         = {
		0x3A, 0x59, 0xF2, 0x38, 0xA2, 0x3E, 0x39, 0x19,
		0x9D, 0xC9, 0x26, 0x66, 0x26, 0xC4, 0x0F, 0x80
	},
	.authsize    = 8,
	.datasize    = 5,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX4 PROGMEM = {
	.name        = "EAX #4",
	.key         = {
		0xD0, 0x7C, 0xF6, 0xCB, 0xB7, 0xF3, 0x13, 0xBD,
		0xDE, 0x66, 0xB7, 0x27, 0xAF, 0xD3, 0xC5, 0xE8
	},
	.plaintext   = {0x48, 0x1C, 0x9E, 0x39, 0xB1},
	.ciphertext  = {0x63, 0x2A, 0x9D, 0x13, 0x1A},
	.authdata    = {0x33, 0xCC, 0xE2, 0xEA, 0xBF, 0xF5, 0xA7, 0x9D},
	.iv          = {
		0x84, 0x08, 0xDF, 0xFF, 0x3C, 0x1A, 0x2B, 0x12,
		0x92, 0xDC, 0x19, 0x9E, 0x46, 0xB7, 0xD6, 0x17
	},
	.tag         = {
		0xD4, 0xC1, 0x68, 0xA4, 0x22, 0x5D, 0x8E, 0x1F,
		0xF7, 0x55, 0x93, 0x99, 0x74, 0xA7, 0xBE, 0xDE
	},
	.authsize    = 8,
	.datasize    = 5,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX5 PROGMEM = {
	.name        = "EAX #5",
	.key         = {
		0x35, 0xB6, 0xD0, 0x58, 0x00, 0x05, 0xBB, 0xC1,
		0x2B, 0x05, 0x87, 0x12, 0x45, 0x57, 0xD2, 0xC2
	},
	.plaintext   = {0x40, 0xD0, 0xC0, 0x7D, 0xA5, 0xE4},
	.ciphertext  = {0x07, 0x1D, 0xFE, 0x16, 0xC6, 0x75},
	.authdata    = {0xAE, 0xB9, 0x6E, 0xAE, 0xBE, 0x29, 0x70, 0xE9},
	.iv          = {
		0xFD, 0xB6, 0xB0, 0x66, 0x76, 0xEE, 0xDC, 0x5C,
		0x61, 0xD7, 0x42, 0x76, 0xE1, 0xF8, 0xE8, 0x16
	},
	.tag         = {
		0xCB, 0x06, 0x77, 0xE5, 0x36, 0xF7, 0x3A, 0xFE,
		0x6A, 0x14, 0xB7, 0x4E, 0xE4, 0x98, 0x44, 0xDD
	},
	.authsize    = 8,
	.datasize    = 6,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX6 PROGMEM = {
	.name        = "EAX #6",
	.key         = {
		0xBD, 0x8E, 0x6E, 0x11, 0x47, 0x5E, 0x60, 0xB2,
		0x68, 0x78, 0x4C, 0x38, 0xC6, 0x2F, 0xEB, 0x22
	},
	.plaintext   = {
		0x4D, 0xE3, 0xB3, 0x5C, 0x3F, 0xC0, 0x39, 0x24,
		0x5B, 0xD1, 0xFB, 0x7D
	},
	.ciphertext  = {
		0x83, 0x5B, 0xB4, 0xF1, 0x5D, 0x74, 0x3E, 0x35,
		0x0E, 0x72, 0x84, 0x14
	},
	.authdata    = {0xD4, 0x48, 0x2D, 0x1C, 0xA7, 0x8D, 0xCE, 0x0F},
	.iv          = {
		0x6E, 0xAC, 0x5C, 0x93, 0x07, 0x2D, 0x8E, 0x85,
		0x13, 0xF7, 0x50, 0x93, 0x5E, 0x46, 0xDA, 0x1B
	},
	.tag         = {
		0xAB, 0xB8, 0x64, 0x4F, 0xD6, 0xCC, 0xB8, 0x69,
		0x47, 0xC5, 0xE1, 0x05, 0x90, 0x21, 0x0A, 0x4F
	},
	.authsize    = 8,
	.datasize    = 12,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX7 PROGMEM = {
	.name        = "EAX #7",
	.key         = {
		0x7C, 0x77, 0xD6, 0xE8, 0x13, 0xBE, 0xD5, 0xAC,
		0x98, 0xBA, 0xA4, 0x17, 0x47, 0x7A, 0x2E, 0x7D
	},
	.plaintext   = {
		0x8B, 0x0A, 0x79, 0x30, 0x6C, 0x9C, 0xE7, 0xED,
		0x99, 0xDA, 0xE4, 0xF8, 0x7F, 0x8D, 0xD6, 0x16,
		0x36
	},
	.ciphertext  = {
		0x02, 0x08, 0x3E, 0x39, 0x79, 0xDA, 0x01, 0x48,
		0x12, 0xF5, 0x9F, 0x11, 0xD5, 0x26, 0x30, 0xDA,
		0x30
	},
	.authdata    = {0x65, 0xD2, 0x01, 0x79, 0x90, 0xD6, 0x25, 0x28},
	.iv          = {
		0x1A, 0x8C, 0x98, 0xDC, 0xD7, 0x3D, 0x38, 0x39,
		0x3B, 0x2B, 0xF1, 0x56, 0x9D, 0xEE, 0xFC, 0x19
	},
	.tag         = {
		0x13, 0x73, 0x27, 0xD1, 0x06, 0x49, 0xB0, 0xAA,
		0x6E, 0x1C, 0x18, 0x1D, 0xB6, 0x17, 0xD7, 0xF2
	},
	.authsize    = 8,
	.datasize    = 17,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX8 PROGMEM = {
	.name        = "EAX #8",
	.key         = {
		0x5F, 0xFF, 0x20, 0xCA, 0xFA, 0xB1, 0x19, 0xCA,
		0x2F, 0xC7, 0x35, 0x49, 0xE2, 0x0F, 0x5B, 0x0D
	},
	.plaintext   = {
		0x1B, 0xDA, 0x12, 0x2B, 0xCE, 0x8A, 0x8D, 0xBA,
		0xF1, 0x87, 0x7D, 0x96, 0x2B, 0x85, 0x92, 0xDD,
		0x2D, 0x56
	},
	.ciphertext  = {
		0x2E, 0xC4, 0x7B, 0x2C, 0x49, 0x54, 0xA4, 0x89,
		0xAF, 0xC7, 0xBA, 0x48, 0x97, 0xED, 0xCD, 0xAE,
		0x8C, 0xC3
	},
	.authdata    = {0x54, 0xB9, 0xF0, 0x4E, 0x6A, 0x09, 0x18, 0x9A},
	.iv          = {
		0xDD, 0xE5, 0x9B, 0x97, 0xD7, 0x22, 0x15, 0x6D,
		0x4D, 0x9A, 0xFF, 0x2B, 0xC7, 0x55, 0x98, 0x26
	},
	.tag         = {
		0x3B, 0x60, 0x45, 0x05, 0x99, 0xBD, 0x02, 0xC9,
		0x63, 0x82, 0x90, 0x2A, 0xEF, 0x7F, 0x83, 0x2A
	},
	.authsize    = 8,
	.datasize    = 18,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX9 PROGMEM = {
	.name        = "EAX #9",
	.key         = {
		0xA4, 0xA4, 0x78, 0x2B, 0xCF, 0xFD, 0x3E, 0xC5,
		0xE7, 0xEF, 0x6D, 0x8C, 0x34, 0xA5, 0x61, 0x23
	},
	.plaintext   = {
		0x6C, 0xF3, 0x67, 0x20, 0x87, 0x2B, 0x85, 0x13,
		0xF6, 0xEA, 0xB1, 0xA8, 0xA4, 0x44, 0x38, 0xD5,
		0xEF, 0x11
	},
	.ciphertext  = {
		0x0D, 0xE1, 0x8F, 0xD0, 0xFD, 0xD9, 0x1E, 0x7A,
		0xF1, 0x9F, 0x1D, 0x8E, 0xE8, 0x73, 0x39, 0x38,
		0xB1, 0xE8
	},
	.authdata    = {0x89, 0x9A, 0x17, 0x58, 0x97, 0x56, 0x1D, 0x7E},
	.iv          = {
		0xB7, 0x81, 0xFC, 0xF2, 0xF7, 0x5F, 0xA5, 0xA8,
		0xDE, 0x97, 0xA9, 0xCA, 0x48, 0xE5, 0x22, 0xEC
	},
	.tag         = {
		0xE7, 0xF6, 0xD2, 0x23, 0x16, 0x18, 0x10, 0x2F,
		0xDB, 0x7F, 0xE5, 0x5F, 0xF1, 0x99, 0x17, 0x00
	},
	.authsize    = 8,
	.datasize    = 18,
	.tagsize     = 16,
	.ivsize      = 16
};
static TestVector const testVectorEAX10 PROGMEM = {
	.name        = "EAX #10",
	.key         = {
		0x83, 0x95, 0xFC, 0xF1, 0xE9, 0x5B, 0xEB, 0xD6,
		0x97, 0xBD, 0x01, 0x0B, 0xC7, 0x66, 0xAA, 0xC3
	},
	.plaintext   = {
		0xCA, 0x40, 0xD7, 0x44, 0x6E, 0x54, 0x5F, 0xFA,
		0xED, 0x3B, 0xD1, 0x2A, 0x74, 0x0A, 0x65, 0x9F,
		0xFB, 0xBB, 0x3C, 0xEA, 0xB7
	},
	.ciphertext  = {
		0xCB, 0x89, 0x20, 0xF8, 0x7A, 0x6C, 0x75, 0xCF,
		0xF3, 0x96, 0x27, 0xB5, 0x6E, 0x3E, 0xD1, 0x97,
		0xC5, 0x52, 0xD2, 0x95, 0xA7
	},
	.authdata    = {0x12, 0x67, 0x35, 0xFC, 0xC3, 0x20, 0xD2, 0x5A},
	.iv          = {
		0x22, 0xE7, 0xAD, 0xD9, 0x3C, 0xFC, 0x63, 0x93,
		0xC5, 0x7E, 0xC0, 0xB3, 0xC1, 0x7D, 0x6B, 0x44
	},
	.tag         = {
		0xCF, 0xC4, 0x6A, 0xFC, 0x25, 0x3B, 0x46, 0x52,
		0xB1, 0xAF, 0x37, 0x95, 0xB1, 0x24, 0xAB, 0x6E
	},
	.authsize    = 8,
	.datasize    = 21,
	.tagsize     = 16,
	.ivsize      = 16
};

TestVector testVector;

EAX<AES128> *eax;
EAX<AES256> *eax256;
EAX<Speck> *eaxSpeck;
EAX<SpeckTiny> *eaxSpeckTiny;

byte buffer[128];

bool testCipher_N(AuthenticatedCipher *cipher, const struct TestVector *test, size_t inc)
{
	size_t posn, len;
	uint8_t tag[16];

	cipher->clear();
	if (!cipher->setKey(test->key, 16)) {
		Serial.print("setKey ");
		return false;
	}
	if (!cipher->setIV(test->iv, test->ivsize)) {
		Serial.print("setIV ");
		return false;
	}

	memset(buffer, 0xBA, sizeof(buffer));

	if (!inc) {
		inc = 1;
	}

	for (posn = 0; posn < test->authsize; posn += inc) {
		len = test->authsize - posn;
		if (len > inc) {
			len = inc;
		}
		cipher->addAuthData(test->authdata + posn, len);
	}

	for (posn = 0; posn < test->datasize; posn += inc) {
		len = test->datasize - posn;
		if (len > inc) {
			len = inc;
		}
		cipher->encrypt(buffer + posn, test->plaintext + posn, len);
	}

	if (memcmp(buffer, test->ciphertext, test->datasize) != 0) {
		Serial.print(buffer[0], HEX);
		Serial.print("->");
		Serial.print(test->ciphertext[0], HEX);
		return false;
	}

	cipher->computeTag(tag, sizeof(tag));
	if (memcmp(tag, test->tag, sizeof(tag)) != 0) {
		Serial.print("computed wrong tag ... ");
		return false;
	}

	cipher->setKey(test->key, 16);
	cipher->setIV(test->iv, test->ivsize);

	for (posn = 0; posn < test->authsize; posn += inc) {
		len = test->authsize - posn;
		if (len > inc) {
			len = inc;
		}
		cipher->addAuthData(test->authdata + posn, len);
	}

	for (posn = 0; posn < test->datasize; posn += inc) {
		len = test->datasize - posn;
		if (len > inc) {
			len = inc;
		}
		cipher->decrypt(buffer + posn, test->ciphertext + posn, len);
	}

	if (memcmp(buffer, test->plaintext, test->datasize) != 0) {
		return false;
	}

	if (!cipher->checkTag(tag, sizeof(tag))) {
		Serial.print("tag did not check ... ");
		return false;
	}

	return true;
}

void testCipher(AuthenticatedCipher *cipher, const struct TestVector *test)
{
	bool ok;

	memcpy_P(&testVector, test, sizeof(TestVector));
	test = &testVector;

	Serial.print(test->name);
	Serial.print(" ... ");

	ok  = testCipher_N(cipher, test, test->datasize);
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

void perfCipherSetKey(AuthenticatedCipher *cipher, const struct TestVector *test,
                      const char *cipherName)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(TestVector));
	test = &testVector;

	Serial.print(cipherName);
	Serial.print(' ');
	Serial.print(test->name);
	Serial.print(" SetKey ... ");

	start = micros();
	for (count = 0; count < 1000; ++count) {
		cipher->setKey(test->key, cipher->keySize());
		cipher->setIV(test->iv, test->ivsize);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / 1000.0);
	Serial.print("us per operation, ");
	Serial.print((1000.0 * 1000000.0) / elapsed);
	Serial.println(" per second");
}

void perfCipherEncrypt(AuthenticatedCipher *cipher, const struct TestVector *test,
                       const char *cipherName)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(TestVector));
	test = &testVector;

	Serial.print(cipherName);
	Serial.print(' ');
	Serial.print(test->name);
	Serial.print(" Encrypt ... ");

	cipher->setKey(test->key, cipher->keySize());
	cipher->setIV(test->iv, test->ivsize);
	start = micros();
	for (count = 0; count < 500; ++count) {
		cipher->encrypt(buffer, buffer, 128);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (128.0 * 500.0));
	Serial.print("us per byte, ");
	Serial.print((128.0 * 500.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");
}

void perfCipherDecrypt(AuthenticatedCipher *cipher, const struct TestVector *test,
                       const char *cipherName)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(TestVector));
	test = &testVector;

	Serial.print(cipherName);
	Serial.print(' ');
	Serial.print(test->name);
	Serial.print(" Decrypt ... ");

	cipher->setKey(test->key, cipher->keySize());
	cipher->setIV(test->iv, test->ivsize);
	start = micros();
	for (count = 0; count < 500; ++count) {
		cipher->decrypt(buffer, buffer, 128);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (128.0 * 500.0));
	Serial.print("us per byte, ");
	Serial.print((128.0 * 500.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");
}

void perfCipherAddAuthData(AuthenticatedCipher *cipher, const struct TestVector *test,
                           const char *cipherName)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(TestVector));
	test = &testVector;

	Serial.print(cipherName);
	Serial.print(' ');
	Serial.print(test->name);
	Serial.print(" AddAuthData ... ");

	cipher->setKey(test->key, cipher->keySize());
	cipher->setIV(test->iv, test->ivsize);
	start = micros();
	memset(buffer, 0xBA, 128);
	for (count = 0; count < 500; ++count) {
		cipher->addAuthData(buffer, 128);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (128.0 * 500.0));
	Serial.print("us per byte, ");
	Serial.print((128.0 * 500.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");
}

void perfCipherComputeTag(AuthenticatedCipher *cipher, const struct TestVector *test,
                          const char *cipherName)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(TestVector));
	test = &testVector;

	Serial.print(cipherName);
	Serial.print(' ');
	Serial.print(test->name);
	Serial.print(" ComputeTag ... ");

	cipher->setKey(test->key, cipher->keySize());
	cipher->setIV(test->iv, test->ivsize);
	start = micros();
	for (count = 0; count < 1000; ++count) {
		cipher->computeTag(buffer, 16);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / 1000.0);
	Serial.print("us per operation, ");
	Serial.print((1000.0 * 1000000.0) / elapsed);
	Serial.println(" per second");
}

void perfCipher(AuthenticatedCipher *cipher, const struct TestVector *test, const char *cipherName)
{
	perfCipherSetKey(cipher, test, cipherName);
	perfCipherEncrypt(cipher, test, cipherName);
	perfCipherDecrypt(cipher, test, cipherName);
	perfCipherAddAuthData(cipher, test, cipherName);
	perfCipherComputeTag(cipher, test, cipherName);
}

void setup()
{
	Serial.begin(9600);

	Serial.println();

	Serial.println("State Sizes:");
	Serial.print("EAX<AES128> ... ");
	Serial.println(sizeof(*eax));
	Serial.print("EAX<AES256> ... ");
	Serial.println(sizeof(*eax256));
	Serial.print("EAX<Speck> ... ");
	Serial.println(sizeof(*eaxSpeck));
	Serial.print("EAX<SpeckTiny> ... ");
	Serial.println(sizeof(*eaxSpeckTiny));
	Serial.println();

	Serial.println("Test Vectors:");
	eax = new EAX<AES128>();
	testCipher(eax, &testVectorEAX1);
	testCipher(eax, &testVectorEAX2);
	testCipher(eax, &testVectorEAX3);
	testCipher(eax, &testVectorEAX4);
	testCipher(eax, &testVectorEAX5);
	testCipher(eax, &testVectorEAX6);
	testCipher(eax, &testVectorEAX7);
	testCipher(eax, &testVectorEAX8);
	testCipher(eax, &testVectorEAX9);
	testCipher(eax, &testVectorEAX10);

	Serial.println();

	Serial.println("Performance Tests:");
	perfCipher(eax, &testVectorEAX1, "AES-128");
	Serial.println();
	delete eax;
	eax256 = new EAX<AES256>();
	perfCipher(eax, &testVectorEAX1, "AES-256");
	Serial.println();
	delete eax256;
	eaxSpeck = new EAX<Speck>();
	perfCipher(eaxSpeck, &testVectorEAX1, "Speck");
	Serial.println();
	delete eaxSpeck;
	eaxSpeckTiny = new EAX<SpeckTiny>();
	perfCipher(eaxSpeckTiny, &testVectorEAX1, "SpeckTiny");
	delete eaxSpeckTiny;
}

void loop()
{
}
