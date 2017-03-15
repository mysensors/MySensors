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
This example runs tests on the Poly1305 implementation to verify correct behaviour.
*/

#include <Crypto.h>
#include <Poly1305.h>
#include <string.h>

// Test vectors from the Poly1305 specification.

struct TestPoly1305Vector {
	const char *name;
	uint8_t key[16];
	uint8_t nonce[16];
	uint8_t data[64];
	size_t dataLen;
	uint8_t hash[16];
};

static TestPoly1305Vector const testVectorPoly1305_1 = {
	.name     = "Poly1305 #1",
	.key      = {
		0x85, 0x1f, 0xc4, 0x0c, 0x34, 0x67, 0xac, 0x0b,
		0xe0, 0x5c, 0xc2, 0x04, 0x04, 0xf3, 0xf7, 0x00
	},
	.nonce    = {
		0x58, 0x0b, 0x3b, 0x0f, 0x94, 0x47, 0xbb, 0x1e,
		0x69, 0xd0, 0x95, 0xb5, 0x92, 0x8b, 0x6d, 0xbc
	},
	.data     = {0xf3, 0xf6},
	.dataLen  = 2,
	.hash     = {
		0xf4, 0xc6, 0x33, 0xc3, 0x04, 0x4f, 0xc1, 0x45,
		0xf8, 0x4f, 0x33, 0x5c, 0xb8, 0x19, 0x53, 0xde
	}

};
static TestPoly1305Vector const testVectorPoly1305_2 = {
	.name     = "Poly1305 #2",
	.key      = {
		0xa0, 0xf3, 0x08, 0x00, 0x00, 0xf4, 0x64, 0x00,
		0xd0, 0xc7, 0xe9, 0x07, 0x6c, 0x83, 0x44, 0x03
	},
	.nonce    = {
		0xdd, 0x3f, 0xab, 0x22, 0x51, 0xf1, 0x1a, 0xc7,
		0x59, 0xf0, 0x88, 0x71, 0x29, 0xcc, 0x2e, 0xe7
	},
	.data     = {0},
	.dataLen  = 0,
	.hash     = {
		0xdd, 0x3f, 0xab, 0x22, 0x51, 0xf1, 0x1a, 0xc7,
		0x59, 0xf0, 0x88, 0x71, 0x29, 0xcc, 0x2e, 0xe7
	}
};
static TestPoly1305Vector const testVectorPoly1305_3 = {
	.name     = "Poly1305 #3",
	.key      = {
		0x48, 0x44, 0x3d, 0x0b, 0xb0, 0xd2, 0x11, 0x09,
		0xc8, 0x9a, 0x10, 0x0b, 0x5c, 0xe2, 0xc2, 0x08
	},
	.nonce    = {
		0x83, 0x14, 0x9c, 0x69, 0xb5, 0x61, 0xdd, 0x88,
		0x29, 0x8a, 0x17, 0x98, 0xb1, 0x07, 0x16, 0xef
	},
	.data     = {
		0x66, 0x3c, 0xea, 0x19, 0x0f, 0xfb, 0x83, 0xd8,
		0x95, 0x93, 0xf3, 0xf4, 0x76, 0xb6, 0xbc, 0x24,
		0xd7, 0xe6, 0x79, 0x10, 0x7e, 0xa2, 0x6a, 0xdb,
		0x8c, 0xaf, 0x66, 0x52, 0xd0, 0x65, 0x61, 0x36
	},
	.dataLen  = 32,
	.hash     = {
		0x0e, 0xe1, 0xc1, 0x6b, 0xb7, 0x3f, 0x0f, 0x4f,
		0xd1, 0x98, 0x81, 0x75, 0x3c, 0x01, 0xcd, 0xbe
	}
};
static TestPoly1305Vector const testVectorPoly1305_4 = {
	.name     = "Poly1305 #4",
	.key      = {
		0x12, 0x97, 0x6a, 0x08, 0xc4, 0x42, 0x6d, 0x0c,
		0xe8, 0xa8, 0x24, 0x07, 0xc4, 0xf4, 0x82, 0x07
	},
	.nonce    = {
		0x80, 0xf8, 0xc2, 0x0a, 0xa7, 0x12, 0x02, 0xd1,
		0xe2, 0x91, 0x79, 0xcb, 0xcb, 0x55, 0x5a, 0x57
	},
	.data     = {
		0xab, 0x08, 0x12, 0x72, 0x4a, 0x7f, 0x1e, 0x34,
		0x27, 0x42, 0xcb, 0xed, 0x37, 0x4d, 0x94, 0xd1,
		0x36, 0xc6, 0xb8, 0x79, 0x5d, 0x45, 0xb3, 0x81,
		0x98, 0x30, 0xf2, 0xc0, 0x44, 0x91, 0xfa, 0xf0,
		0x99, 0x0c, 0x62, 0xe4, 0x8b, 0x80, 0x18, 0xb2,
		0xc3, 0xe4, 0xa0, 0xfa, 0x31, 0x34, 0xcb, 0x67,
		0xfa, 0x83, 0xe1, 0x58, 0xc9, 0x94, 0xd9, 0x61,
		0xc4, 0xcb, 0x21, 0x09, 0x5c, 0x1b, 0xf9
	},
	.dataLen  = 63,
	.hash     = {
		0x51, 0x54, 0xad, 0x0d, 0x2c, 0xb2, 0x6e, 0x01,
		0x27, 0x4f, 0xc5, 0x11, 0x48, 0x49, 0x1f, 0x1b
	}
};

Poly1305 poly1305;

byte buffer[128];

bool testPoly1305_N(Poly1305 *hash, const struct TestPoly1305Vector *test, size_t inc)
{
	size_t size = test->dataLen;
	size_t posn, len;

	hash->reset(test->key);

	for (posn = 0; posn < size; posn += inc) {
		len = size - posn;
		if (len > inc) {
			len = inc;
		}
		hash->update(test->data + posn, len);
	}

	hash->finalize(test->nonce, buffer, 16);

	return !memcmp(buffer, test->hash, 16);
}

void testPoly1305(Poly1305 *hash, const struct TestPoly1305Vector *test)
{
	bool ok;

	Serial.print(test->name);
	Serial.print(" ... ");

	ok  = testPoly1305_N(hash, test, test->dataLen);
	ok &= testPoly1305_N(hash, test, 1);
	ok &= testPoly1305_N(hash, test, 2);
	ok &= testPoly1305_N(hash, test, 5);
	ok &= testPoly1305_N(hash, test, 8);
	ok &= testPoly1305_N(hash, test, 13);
	ok &= testPoly1305_N(hash, test, 16);
	ok &= testPoly1305_N(hash, test, 24);
	ok &= testPoly1305_N(hash, test, 63);
	ok &= testPoly1305_N(hash, test, 64);

	if (ok) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
	}
}

void perfPoly1305(Poly1305 *hash)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print("Hashing ... ");

	for (size_t posn = 0; posn < sizeof(buffer); ++posn) {
		buffer[posn] = (uint8_t)posn;
	}

	hash->reset(testVectorPoly1305_1.key);
	start = micros();
	for (count = 0; count < 1000; ++count) {
		hash->update(buffer, sizeof(buffer));
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (sizeof(buffer) * 1000.0));
	Serial.print("us per byte, ");
	Serial.print((sizeof(buffer) * 1000.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");
}

void perfPoly1305SetKey(Poly1305 *hash)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print("Set Key ... ");

	start = micros();
	for (count = 0; count < 1000; ++count) {
		hash->reset(testVectorPoly1305_1.key);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / 1000.0);
	Serial.print("us per op, ");
	Serial.print((1000.0 * 1000000.0) / elapsed);
	Serial.println(" ops per second");
}

void perfPoly1305Finalize(Poly1305 *hash)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print("Finalize ... ");

	hash->reset(testVectorPoly1305_1.key);
	hash->update("abc", 3);
	start = micros();
	for (count = 0; count < 1000; ++count) {
		hash->finalize(testVectorPoly1305_1.nonce, buffer, 16);
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
	Serial.println(sizeof(Poly1305));
	Serial.println();

	Serial.println("Test Vectors:");
	testPoly1305(&poly1305, &testVectorPoly1305_1);
	testPoly1305(&poly1305, &testVectorPoly1305_2);
	testPoly1305(&poly1305, &testVectorPoly1305_3);
	testPoly1305(&poly1305, &testVectorPoly1305_4);

	Serial.println();

	Serial.println("Performance Tests:");
	perfPoly1305(&poly1305);
	perfPoly1305SetKey(&poly1305);
	perfPoly1305Finalize(&poly1305);
}

void loop()
{
}
