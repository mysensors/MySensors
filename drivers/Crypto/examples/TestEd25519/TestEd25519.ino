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
This example runs tests on the Ed25519 algorithm.
*/

#include <Crypto.h>
#include <Ed25519.h>
#include <RNG.h>
#include <utility/ProgMemUtil.h>
#include <string.h>

struct TestVector {
	const char *name;
	uint8_t privateKey[32];
	uint8_t publicKey[32];
	uint8_t message[2];
	size_t len;
	uint8_t signature[64];
};

// Test vectors for Ed25519 from:
// https://tools.ietf.org/html/draft-irtf-cfrg-eddsa-05
static TestVector const testVectorEd25519_1 PROGMEM = {
	.name       = "Ed25519 #1",
	.privateKey = {
		0x9d, 0x61, 0xb1, 0x9d, 0xef, 0xfd, 0x5a, 0x60,
		0xba, 0x84, 0x4a, 0xf4, 0x92, 0xec, 0x2c, 0xc4,
		0x44, 0x49, 0xc5, 0x69, 0x7b, 0x32, 0x69, 0x19,
		0x70, 0x3b, 0xac, 0x03, 0x1c, 0xae, 0x7f, 0x60
	},
	.publicKey  = {
		0xd7, 0x5a, 0x98, 0x01, 0x82, 0xb1, 0x0a, 0xb7,
		0xd5, 0x4b, 0xfe, 0xd3, 0xc9, 0x64, 0x07, 0x3a,
		0x0e, 0xe1, 0x72, 0xf3, 0xda, 0xa6, 0x23, 0x25,
		0xaf, 0x02, 0x1a, 0x68, 0xf7, 0x07, 0x51, 0x1a
	},
	.message    = {0x00, 0x00},
	.len        = 0,
	.signature  = {
		0xe5, 0x56, 0x43, 0x00, 0xc3, 0x60, 0xac, 0x72,
		0x90, 0x86, 0xe2, 0xcc, 0x80, 0x6e, 0x82, 0x8a,
		0x84, 0x87, 0x7f, 0x1e, 0xb8, 0xe5, 0xd9, 0x74,
		0xd8, 0x73, 0xe0, 0x65, 0x22, 0x49, 0x01, 0x55,
		0x5f, 0xb8, 0x82, 0x15, 0x90, 0xa3, 0x3b, 0xac,
		0xc6, 0x1e, 0x39, 0x70, 0x1c, 0xf9, 0xb4, 0x6b,
		0xd2, 0x5b, 0xf5, 0xf0, 0x59, 0x5b, 0xbe, 0x24,
		0x65, 0x51, 0x41, 0x43, 0x8e, 0x7a, 0x10, 0x0b
	}
};
static TestVector const testVectorEd25519_2 PROGMEM = {
	.name       = "Ed25519 #2",
	.privateKey = {
		0x4c, 0xcd, 0x08, 0x9b, 0x28, 0xff, 0x96, 0xda,
		0x9d, 0xb6, 0xc3, 0x46, 0xec, 0x11, 0x4e, 0x0f,
		0x5b, 0x8a, 0x31, 0x9f, 0x35, 0xab, 0xa6, 0x24,
		0xda, 0x8c, 0xf6, 0xed, 0x4f, 0xb8, 0xa6, 0xfb
	},
	.publicKey  = {
		0x3d, 0x40, 0x17, 0xc3, 0xe8, 0x43, 0x89, 0x5a,
		0x92, 0xb7, 0x0a, 0xa7, 0x4d, 0x1b, 0x7e, 0xbc,
		0x9c, 0x98, 0x2c, 0xcf, 0x2e, 0xc4, 0x96, 0x8c,
		0xc0, 0xcd, 0x55, 0xf1, 0x2a, 0xf4, 0x66, 0x0c
	},
	.message    = {0x72, 0x00},
	.len        = 1,
	.signature  = {
		0x92, 0xa0, 0x09, 0xa9, 0xf0, 0xd4, 0xca, 0xb8,
		0x72, 0x0e, 0x82, 0x0b, 0x5f, 0x64, 0x25, 0x40,
		0xa2, 0xb2, 0x7b, 0x54, 0x16, 0x50, 0x3f, 0x8f,
		0xb3, 0x76, 0x22, 0x23, 0xeb, 0xdb, 0x69, 0xda,
		0x08, 0x5a, 0xc1, 0xe4, 0x3e, 0x15, 0x99, 0x6e,
		0x45, 0x8f, 0x36, 0x13, 0xd0, 0xf1, 0x1d, 0x8c,
		0x38, 0x7b, 0x2e, 0xae, 0xb4, 0x30, 0x2a, 0xee,
		0xb0, 0x0d, 0x29, 0x16, 0x12, 0xbb, 0x0c, 0x00
	}
};

static TestVector testVector;

void printNumber(const char *name, const uint8_t *x, uint8_t len)
{
	static const char hexchars[] = "0123456789ABCDEF";
	Serial.print(name);
	Serial.print(" = ");
	for (uint8_t posn = 0; posn < len; ++posn) {
		Serial.print(hexchars[(x[posn] >> 4) & 0x0F]);
		Serial.print(hexchars[x[posn] & 0x0F]);
	}
	Serial.println();
}

void testFixedVectors(const struct TestVector *test)
{
	// Copy the test vector out of program memory.
	memcpy_P(&testVector, test, sizeof(TestVector));
	test = &testVector;

	// Sign using the test vector.
	uint8_t signature[64];
	Serial.print(test->name);
	Serial.print(" sign ... ");
	Serial.flush();
	unsigned long start = micros();
	Ed25519::sign(signature, test->privateKey, test->publicKey,
	              test->message, test->len);
	unsigned long elapsed = micros() - start;
	if (memcmp(signature, test->signature, 64) == 0) {
		Serial.print("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", signature, 64);
		printNumber("expected", test->signature, 64);
	}
	Serial.print(" (elapsed ");
	Serial.print(elapsed);
	Serial.println(" us)");

	// Verify using the test vector.
	Serial.print(test->name);
	Serial.print(" verify ... ");
	Serial.flush();
	start = micros();
	bool verified = Ed25519::verify(signature, test->publicKey, test->message, test->len);
	elapsed = micros() - start;
	if (verified) {
		Serial.print("ok");
	} else {
		Serial.println("failed");
	}
	Serial.print(" (elapsed ");
	Serial.print(elapsed);
	Serial.println(" us)");

	// Check derivation of the public key from the private key.
	Serial.print(test->name);
	Serial.print(" derive public key ... ");
	Serial.flush();
	start = micros();
	Ed25519::derivePublicKey(signature, test->privateKey);
	elapsed = micros() - start;
	if (memcmp(signature, test->publicKey, 32) == 0) {
		Serial.print("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", signature, 32);
		printNumber("expected", test->publicKey, 32);
	}
	Serial.print(" (elapsed ");
	Serial.print(elapsed);
	Serial.println(" us)");
}

void testFixedVectors()
{
	testFixedVectors(&testVectorEd25519_1);
	testFixedVectors(&testVectorEd25519_2);
}

void setup()
{
	Serial.begin(9600);

	// Start the random number generator.  We don't initialise a noise
	// source here because we don't need one for testing purposes.
	// Real applications should of course use a proper noise source.
	RNG.begin("TestEd25519 1.0", 950);

	// Perform the tests.
	testFixedVectors();
	Serial.println();
}

void loop()
{
}
