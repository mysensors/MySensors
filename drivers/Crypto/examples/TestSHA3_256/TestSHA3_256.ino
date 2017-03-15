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
This example runs tests on the SHA3_256 implementation to verify
correct behaviour.
*/

#include <Crypto.h>
#include <SHA3.h>
#include <string.h>

#define DATA_SIZE 136
#define HASH_SIZE 32
#define BLOCK_SIZE 136

struct TestHashVector {
	const char *name;
	uint8_t data[DATA_SIZE];
	uint8_t dataSize;
	uint8_t hash[HASH_SIZE];
};

// Some test vectors from https://github.com/gvanas/KeccakCodePackage
static TestHashVector const testVectorSHA3_256_1 = {
	"SHA3-256 #1",
	{0},
	0,
	{
		0xA7, 0xFF, 0xC6, 0xF8, 0xBF, 0x1E, 0xD7, 0x66,
		0x51, 0xC1, 0x47, 0x56, 0xA0, 0x61, 0xD6, 0x62,
		0xF5, 0x80, 0xFF, 0x4D, 0xE4, 0x3B, 0x49, 0xFA,
		0x82, 0xD8, 0x0A, 0x4B, 0x80, 0xF8, 0x43, 0x4A
	}
};
static TestHashVector const testVectorSHA3_256_2 = {
	"SHA3-256 #2",
	{0x1F, 0x87, 0x7C},
	3,
	{
		0xBC, 0x22, 0x34, 0x5E, 0x4B, 0xD3, 0xF7, 0x92,
		0xA3, 0x41, 0xCF, 0x18, 0xAC, 0x07, 0x89, 0xF1,
		0xC9, 0xC9, 0x66, 0x71, 0x2A, 0x50, 0x1B, 0x19,
		0xD1, 0xB6, 0x63, 0x2C, 0xCD, 0x40, 0x8E, 0xC5
	}
};
static TestHashVector const testVectorSHA3_256_3 = {
	"SHA3-256 #3",
	{
		0xE2, 0x61, 0x93, 0x98, 0x9D, 0x06, 0x56, 0x8F,
		0xE6, 0x88, 0xE7, 0x55, 0x40, 0xAE, 0xA0, 0x67,
		0x47, 0xD9, 0xF8, 0x51
	},
	20,
	{
		0x2C, 0x1E, 0x61, 0xE5, 0xD4, 0x52, 0x03, 0xF2,
		0x7B, 0x86, 0xF1, 0x29, 0x3A, 0x80, 0xBA, 0xB3,
		0x41, 0x92, 0xDA, 0xF4, 0x2B, 0x86, 0x23, 0xB1,
		0x20, 0x05, 0xB2, 0xFB, 0x1C, 0x18, 0xAC, 0xB1
	}
};
static TestHashVector const testVectorSHA3_256_4 = {
	"SHA3-256 #4",
	{
		0xB7, 0x71, 0xD5, 0xCE, 0xF5, 0xD1, 0xA4, 0x1A,
		0x93, 0xD1, 0x56, 0x43, 0xD7, 0x18, 0x1D, 0x2A,
		0x2E, 0xF0, 0xA8, 0xE8, 0x4D, 0x91, 0x81, 0x2F,
		0x20, 0xED, 0x21, 0xF1, 0x47, 0xBE, 0xF7, 0x32,
		0xBF, 0x3A, 0x60, 0xEF, 0x40, 0x67, 0xC3, 0x73,
		0x4B, 0x85, 0xBC, 0x8C, 0xD4, 0x71, 0x78, 0x0F,
		0x10, 0xDC, 0x9E, 0x82, 0x91, 0xB5, 0x83, 0x39,
		0xA6, 0x77, 0xB9, 0x60, 0x21, 0x8F, 0x71, 0xE7,
		0x93, 0xF2, 0x79, 0x7A, 0xEA, 0x34, 0x94, 0x06,
		0x51, 0x28, 0x29, 0x06, 0x5D, 0x37, 0xBB, 0x55,
		0xEA, 0x79, 0x6F, 0xA4, 0xF5, 0x6F, 0xD8, 0x89,
		0x6B, 0x49, 0xB2, 0xCD, 0x19, 0xB4, 0x32, 0x15,
		0xAD, 0x96, 0x7C, 0x71, 0x2B, 0x24, 0xE5, 0x03,
		0x2D, 0x06, 0x52, 0x32, 0xE0, 0x2C, 0x12, 0x74,
		0x09, 0xD2, 0xED, 0x41, 0x46, 0xB9, 0xD7, 0x5D,
		0x76, 0x3D, 0x52, 0xDB, 0x98, 0xD9, 0x49, 0xD3,
		0xB0, 0xFE, 0xD6, 0xA8, 0x05, 0x2F, 0xBB
	},
	135,
	{
		0xA1, 0x9E, 0xEE, 0x92, 0xBB, 0x20, 0x97, 0xB6,
		0x4E, 0x82, 0x3D, 0x59, 0x77, 0x98, 0xAA, 0x18,
		0xBE, 0x9B, 0x7C, 0x73, 0x6B, 0x80, 0x59, 0xAB,
		0xFD, 0x67, 0x79, 0xAC, 0x35, 0xAC, 0x81, 0xB5
	}
};
static TestHashVector testVectorSHA3_256_5 = {
	"SHA3-256 #5",
	{
		0xB3, 0x2D, 0x95, 0xB0, 0xB9, 0xAA, 0xD2, 0xA8,
		0x81, 0x6D, 0xE6, 0xD0, 0x6D, 0x1F, 0x86, 0x00,
		0x85, 0x05, 0xBD, 0x8C, 0x14, 0x12, 0x4F, 0x6E,
		0x9A, 0x16, 0x3B, 0x5A, 0x2A, 0xDE, 0x55, 0xF8,
		0x35, 0xD0, 0xEC, 0x38, 0x80, 0xEF, 0x50, 0x70,
		0x0D, 0x3B, 0x25, 0xE4, 0x2C, 0xC0, 0xAF, 0x05,
		0x0C, 0xCD, 0x1B, 0xE5, 0xE5, 0x55, 0xB2, 0x30,
		0x87, 0xE0, 0x4D, 0x7B, 0xF9, 0x81, 0x36, 0x22,
		0x78, 0x0C, 0x73, 0x13, 0xA1, 0x95, 0x4F, 0x87,
		0x40, 0xB6, 0xEE, 0x2D, 0x3F, 0x71, 0xF7, 0x68,
		0xDD, 0x41, 0x7F, 0x52, 0x04, 0x82, 0xBD, 0x3A,
		0x08, 0xD4, 0xF2, 0x22, 0xB4, 0xEE, 0x9D, 0xBD,
		0x01, 0x54, 0x47, 0xB3, 0x35, 0x07, 0xDD, 0x50,
		0xF3, 0xAB, 0x42, 0x47, 0xC5, 0xDE, 0x9A, 0x8A,
		0xBD, 0x62, 0xA8, 0xDE, 0xCE, 0xA0, 0x1E, 0x3B,
		0x87, 0xC8, 0xB9, 0x27, 0xF5, 0xB0, 0x8B, 0xEB,
		0x37, 0x67, 0x4C, 0x6F, 0x8E, 0x38, 0x0C, 0x04
	},
	136,
	{
		0xDF, 0x67, 0x3F, 0x41, 0x05, 0x37, 0x9F, 0xF6,
		0xB7, 0x55, 0xEE, 0xAB, 0x20, 0xCE, 0xB0, 0xDC,
		0x77, 0xB5, 0x28, 0x63, 0x64, 0xFE, 0x16, 0xC5,
		0x9C, 0xC8, 0xA9, 0x07, 0xAF, 0xF0, 0x77, 0x32
	}
};

SHA3_256 sha3_256;

bool testHash_N(Hash *hash, const struct TestHashVector *test, size_t inc)
{
	size_t size = test->dataSize;
	size_t posn, len;
	uint8_t value[HASH_SIZE];

	hash->reset();
	for (posn = 0; posn < size; posn += inc) {
		len = size - posn;
		if (len > inc) {
			len = inc;
		}
		hash->update(test->data + posn, len);
	}
	hash->finalize(value, sizeof(value));
	if (memcmp(value, test->hash, sizeof(value)) != 0) {
		return false;
	}

	return true;
}

void testHash(Hash *hash, const struct TestHashVector *test)
{
	bool ok;

	Serial.print(test->name);
	Serial.print(" ... ");

	ok  = testHash_N(hash, test, test->dataSize);
	ok &= testHash_N(hash, test, 1);
	ok &= testHash_N(hash, test, 2);
	ok &= testHash_N(hash, test, 5);
	ok &= testHash_N(hash, test, 8);
	ok &= testHash_N(hash, test, 13);
	ok &= testHash_N(hash, test, 16);
	ok &= testHash_N(hash, test, 24);
	ok &= testHash_N(hash, test, 63);
	ok &= testHash_N(hash, test, 64);

	if (ok) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
	}
}

void perfHash(Hash *hash)
{
	unsigned long start;
	unsigned long elapsed;
	int count;
	// Reuse one of the test vectors as a large temporary buffer.
	uint8_t *buffer = (uint8_t *)&testVectorSHA3_256_5;

	Serial.print("Hashing ... ");

	for (size_t posn = 0; posn < 128; ++posn) {
		buffer[posn] = (uint8_t)posn;
	}

	hash->reset();
	start = micros();
	for (count = 0; count < 500; ++count) {
		hash->update(buffer, 128);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (128 * 500.0));
	Serial.print("us per byte, ");
	Serial.print((128 * 500.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");
}

// Very simple method for hashing a HMAC inner or outer key.
void hashKey(Hash *hash, const uint8_t *key, size_t keyLen, uint8_t pad)
{
	size_t posn;
	uint8_t buf;
	uint8_t result[HASH_SIZE];
	if (keyLen <= BLOCK_SIZE) {
		hash->reset();
		for (posn = 0; posn < BLOCK_SIZE; ++posn) {
			if (posn < keyLen) {
				buf = key[posn] ^ pad;
			} else {
				buf = pad;
			}
			hash->update(&buf, 1);
		}
	} else {
		hash->reset();
		hash->update(key, keyLen);
		hash->finalize(result, HASH_SIZE);
		hash->reset();
		for (posn = 0; posn < BLOCK_SIZE; ++posn) {
			if (posn < HASH_SIZE) {
				buf = result[posn] ^ pad;
			} else {
				buf = pad;
			}
			hash->update(&buf, 1);
		}
	}
}

void testHMAC(Hash *hash, size_t keyLen)
{
	uint8_t result[HASH_SIZE];
	// Reuse one of the test vectors as a large temporary buffer.
	uint8_t *buffer = (uint8_t *)&testVectorSHA3_256_5;

	Serial.print("HMAC-SHA3-256 keysize=");
	Serial.print(keyLen);
	Serial.print(" ... ");

	// Construct the expected result with a simple HMAC implementation.
	memset(buffer, (uint8_t)keyLen, keyLen);
	hashKey(hash, buffer, keyLen, 0x36);
	memset(buffer, 0xBA, sizeof(testVectorSHA3_256_5));
	hash->update(buffer, sizeof(testVectorSHA3_256_5));
	hash->finalize(result, HASH_SIZE);
	memset(buffer, (uint8_t)keyLen, keyLen);
	hashKey(hash, buffer, keyLen, 0x5C);
	hash->update(result, HASH_SIZE);
	hash->finalize(result, HASH_SIZE);

	// Now use the library to compute the HMAC.
	hash->resetHMAC(buffer, keyLen);
	memset(buffer, 0xBA, sizeof(testVectorSHA3_256_5));
	hash->update(buffer, sizeof(testVectorSHA3_256_5));
	memset(buffer, (uint8_t)keyLen, keyLen);
	hash->finalizeHMAC(buffer, keyLen, buffer, HASH_SIZE);

	// Check the result.
	if (!memcmp(result, buffer, HASH_SIZE)) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
	}
}

void perfFinalize(Hash *hash)
{
	unsigned long start;
	unsigned long elapsed;
	int count;
	// Reuse one of the test vectors as a large temporary buffer.
	uint8_t *buffer = (uint8_t *)&testVectorSHA3_256_5;

	Serial.print("Finalizing ... ");

	hash->reset();
	hash->update("abc", 3);
	start = micros();
	for (count = 0; count < 1000; ++count) {
		hash->finalize(buffer, hash->hashSize());
	}
	elapsed = micros() - start;

	Serial.print(elapsed / 1000.0);
	Serial.print("us per op, ");
	Serial.print((1000.0 * 1000000.0) / elapsed);
	Serial.println(" ops per second");
}

void setup()
{
	Serial.begin(9600);

	Serial.println();

	Serial.print("State Size ...");
	Serial.println(sizeof(SHA3_256));
	Serial.println();

	Serial.println("Test Vectors:");
	testHash(&sha3_256, &testVectorSHA3_256_1);
	testHash(&sha3_256, &testVectorSHA3_256_2);
	testHash(&sha3_256, &testVectorSHA3_256_3);
	testHash(&sha3_256, &testVectorSHA3_256_4);
	testHash(&sha3_256, &testVectorSHA3_256_5);
	testHMAC(&sha3_256, (size_t)0);
	testHMAC(&sha3_256, 1);
	testHMAC(&sha3_256, HASH_SIZE);
	testHMAC(&sha3_256, BLOCK_SIZE);
	testHMAC(&sha3_256, BLOCK_SIZE + 1);
	testHMAC(&sha3_256, BLOCK_SIZE + 2);

	Serial.println();

	Serial.println("Performance Tests:");
	perfHash(&sha3_256);
	perfFinalize(&sha3_256);
}

void loop()
{
}
