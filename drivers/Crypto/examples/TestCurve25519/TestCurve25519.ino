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
This example runs tests on the Curve25519 algorithm.
*/

#include <Crypto.h>
#include <Curve25519.h>
#include <RNG.h>
#include <string.h>

void printNumber(const char *name, const uint8_t *x)
{
	static const char hexchars[] = "0123456789ABCDEF";
	Serial.print(name);
	Serial.print(" = ");
	for (uint8_t posn = 0; posn < 32; ++posn) {
		Serial.print(hexchars[(x[posn] >> 4) & 0x0F]);
		Serial.print(hexchars[x[posn] & 0x0F]);
	}
	Serial.println();
}

// Check the eval() function using the test vectors from
// section 6.1 of RFC 7748.
void testEval()
{
	static uint8_t alice_private[32] = {
		0x77, 0x07, 0x6d, 0x0a, 0x73, 0x18, 0xa5, 0x7d,
		0x3c, 0x16, 0xc1, 0x72, 0x51, 0xb2, 0x66, 0x45,
		0xdf, 0x4c, 0x2f, 0x87, 0xeb, 0xc0, 0x99, 0x2a,
		0xb1, 0x77, 0xfb, 0xa5, 0x1d, 0xb9, 0x2c, 0x2a
	};
	static uint8_t const alice_public[32] = {
		0x85, 0x20, 0xf0, 0x09, 0x89, 0x30, 0xa7, 0x54,
		0x74, 0x8b, 0x7d, 0xdc, 0xb4, 0x3e, 0xf7, 0x5a,
		0x0d, 0xbf, 0x3a, 0x0d, 0x26, 0x38, 0x1a, 0xf4,
		0xeb, 0xa4, 0xa9, 0x8e, 0xaa, 0x9b, 0x4e, 0x6a
	};
	static uint8_t bob_private[32] = {
		0x5d, 0xab, 0x08, 0x7e, 0x62, 0x4a, 0x8a, 0x4b,
		0x79, 0xe1, 0x7f, 0x8b, 0x83, 0x80, 0x0e, 0xe6,
		0x6f, 0x3b, 0xb1, 0x29, 0x26, 0x18, 0xb6, 0xfd,
		0x1c, 0x2f, 0x8b, 0x27, 0xff, 0x88, 0xe0, 0xeb
	};
	static uint8_t const bob_public[32] = {
		0xde, 0x9e, 0xdb, 0x7d, 0x7b, 0x7d, 0xc1, 0xb4,
		0xd3, 0x5b, 0x61, 0xc2, 0xec, 0xe4, 0x35, 0x37,
		0x3f, 0x83, 0x43, 0xc8, 0x5b, 0x78, 0x67, 0x4d,
		0xad, 0xfc, 0x7e, 0x14, 0x6f, 0x88, 0x2b, 0x4f
	};
	static uint8_t const shared_secret[32] = {
		0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1,
		0x72, 0x8e, 0x3b, 0xf4, 0x80, 0x35, 0x0f, 0x25,
		0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1, 0x9e, 0x33,
		0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42
	};

	// Fix up the private keys by applying the standard masks.
	alice_private[0] &= 0xF8;
	alice_private[31] = (alice_private[31] & 0x7F) | 0x40;
	bob_private[0] &= 0xF8;
	bob_private[31] = (bob_private[31] & 0x7F) | 0x40;

	// Evaluate the curve function and check the public keys.
	uint8_t result[32];
	Serial.println("Fixed test vectors:");
	Serial.print("Computing Alice's public key ... ");
	Serial.flush();
	unsigned long start = micros();
	Curve25519::eval(result, alice_private, 0);
	unsigned long elapsed = micros() - start;
	if (memcmp(result, alice_public, 32) == 0) {
		Serial.print("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", result);
		printNumber("expected", alice_public);
	}
	Serial.print(" (elapsed ");
	Serial.print(elapsed);
	Serial.println(" us)");
	Serial.print("Computing Bob's public key ... ");
	Serial.flush();
	start = micros();
	Curve25519::eval(result, bob_private, 0);
	elapsed = micros() - start;
	if (memcmp(result, bob_public, 32) == 0) {
		Serial.print("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", result);
		printNumber("expected", bob_public);
	}
	Serial.print(" (elapsed ");
	Serial.print(elapsed);
	Serial.println(" us)");

	// Compute the shared secret from each side.
	Serial.print("Computing Alice's shared secret ... ");
	Serial.flush();
	start = micros();
	Curve25519::eval(result, alice_private, bob_public);
	elapsed = micros() - start;
	if (memcmp(result, shared_secret, 32) == 0) {
		Serial.print("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", result);
		printNumber("expected", shared_secret);
	}
	Serial.print(" (elapsed ");
	Serial.print(elapsed);
	Serial.println(" us)");
	Serial.print("Computing Bob's shared secret ... ");
	Serial.flush();
	start = micros();
	Curve25519::eval(result, bob_private, alice_public);
	elapsed = micros() - start;
	if (memcmp(result, shared_secret, 32) == 0) {
		Serial.print("ok");
	} else {
		Serial.println("failed");
		printNumber("actual  ", result);
		printNumber("expected", shared_secret);
	}
	Serial.print(" (elapsed ");
	Serial.print(elapsed);
	Serial.println(" us)");
}

void testDH()
{
	static uint8_t alice_k[32];
	static uint8_t alice_f[32];
	static uint8_t bob_k[32];
	static uint8_t bob_f[32];

	Serial.println("Diffie-Hellman key exchange:");
	Serial.print("Generate random k/f for Alice ... ");
	Serial.flush();
	unsigned long start = micros();
	Curve25519::dh1(alice_k, alice_f);
	unsigned long elapsed = micros() - start;
	Serial.print("elapsed ");
	Serial.print(elapsed);
	Serial.println(" us");

	Serial.print("Generate random k/f for Bob ... ");
	Serial.flush();
	start = micros();
	Curve25519::dh1(bob_k, bob_f);
	elapsed = micros() - start;
	Serial.print("elapsed ");
	Serial.print(elapsed);
	Serial.println(" us");

	Serial.print("Generate shared secret for Alice ... ");
	Serial.flush();
	start = micros();
	Curve25519::dh2(bob_k, alice_f);
	elapsed = micros() - start;
	Serial.print("elapsed ");
	Serial.print(elapsed);
	Serial.println(" us");

	Serial.print("Generate shared secret for Bob ... ");
	Serial.flush();
	start = micros();
	Curve25519::dh2(alice_k, bob_f);
	elapsed = micros() - start;
	Serial.print("elapsed ");
	Serial.print(elapsed);
	Serial.println(" us");

	Serial.print("Check that the shared secrets match ... ");
	if (memcmp(alice_k, bob_k, 32) == 0) {
		Serial.println("ok");
	} else {
		Serial.println("failed");
	}
}

void setup()
{
	Serial.begin(9600);

	// Start the random number generator.  We don't initialise a noise
	// source here because we don't need one for testing purposes.
	// Real DH applications should of course use a proper noise source.
	RNG.begin("TestCurve25519 1.0", 950);

	// Perform the tests.
	testEval();
	Serial.println();
	testDH();
	Serial.println();
}

void loop()
{
}
