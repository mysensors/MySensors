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
This example runs tests on the Speck implementation to verify correct behaviour.
*/

#include <Crypto.h>
#include <Speck.h>
#include <SpeckSmall.h>
#include <SpeckTiny.h>
#include <string.h>

struct TestVector {
	const char *name;
	byte key[32];
	byte plaintext[16];
	byte ciphertext[16];
};

// Define the test vectors from http://eprint.iacr.org/2013/404
static TestVector const testVectorSpeck128 = {
	.name        = "Speck-128-ECB",
	.key         = {
		0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
		0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	},
	.plaintext   = {
		0x6c, 0x61, 0x76, 0x69, 0x75, 0x71, 0x65, 0x20,
		0x74, 0x69, 0x20, 0x65, 0x64, 0x61, 0x6d, 0x20
	},
	.ciphertext  = {
		0xa6, 0x5d, 0x98, 0x51, 0x79, 0x78, 0x32, 0x65,
		0x78, 0x60, 0xfe, 0xdf, 0x5c, 0x57, 0x0d, 0x18
	}
};
static TestVector const testVectorSpeck192 = {
	.name        = "Speck-192-ECB",
	.key         = {
		0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
		0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
		0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	},
	.plaintext   = {
		0x72, 0x61, 0x48, 0x20, 0x66, 0x65, 0x69, 0x68,
		0x43, 0x20, 0x6f, 0x74, 0x20, 0x74, 0x6e, 0x65
	},
	.ciphertext  = {
		0x1b, 0xe4, 0xcf, 0x3a, 0x13, 0x13, 0x55, 0x66,
		0xf9, 0xbc, 0x18, 0x5d, 0xe0, 0x3c, 0x18, 0x86
	}
};
static TestVector const testVectorSpeck256 = {
	.name        = "Speck-256-ECB",
	.key         = {
		0x1f, 0x1e, 0x1d, 0x1c, 0x1b, 0x1a, 0x19, 0x18,
		0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
		0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
		0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	},
	.plaintext   = {
		0x65, 0x73, 0x6f, 0x68, 0x74, 0x20, 0x6e, 0x49,
		0x20, 0x2e, 0x72, 0x65, 0x6e, 0x6f, 0x6f, 0x70
	},
	.ciphertext  = {
		0x41, 0x09, 0x01, 0x04, 0x05, 0xc0, 0xf5, 0x3e,
		0x4e, 0xee, 0xb4, 0x8d, 0x9c, 0x18, 0x8f, 0x43
	}
};

Speck speck;
SpeckSmall speckSmall;
SpeckTiny speckTiny;

byte buffer[16];

void testCipher(BlockCipher *cipher, const struct TestVector *test, size_t keySize,
                bool decryption = true)
{
	Serial.print(test->name);
	Serial.print(" Encryption ... ");
	cipher->setKey(test->key, keySize);
	cipher->encryptBlock(buffer, test->plaintext);
	if (memcmp(buffer, test->ciphertext, 16) == 0) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
	}

	if (!decryption) {
		return;
	}

	Serial.print(test->name);
	Serial.print(" Decryption ... ");
	cipher->decryptBlock(buffer, test->ciphertext);
	if (memcmp(buffer, test->plaintext, 16) == 0) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
	}
}

void perfCipher(BlockCipher *cipher, const struct TestVector *test, size_t keySize,
                bool decryption = true)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print(test->name);
	Serial.print(" Set Key ... ");
	start = micros();
	for (count = 0; count < 10000; ++count) {
		cipher->setKey(test->key, keySize);
	}
	elapsed = micros() - start;
	Serial.print(elapsed / 10000.0);
	Serial.print("us per operation, ");
	Serial.print((10000.0 * 1000000.0) / elapsed);
	Serial.println(" per second");

	Serial.print(test->name);
	Serial.print(" Encrypt ... ");
	start = micros();
	for (count = 0; count < 5000; ++count) {
		cipher->encryptBlock(buffer, buffer);
	}
	elapsed = micros() - start;
	Serial.print(elapsed / (5000.0 * 16.0));
	Serial.print("us per byte, ");
	Serial.print((16.0 * 5000.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");

	if (!decryption) {
		Serial.println();
		return;
	}

	Serial.print(test->name);
	Serial.print(" Decrypt ... ");
	start = micros();
	for (count = 0; count < 5000; ++count) {
		cipher->decryptBlock(buffer, buffer);
	}
	elapsed = micros() - start;
	Serial.print(elapsed / (5000.0 * 16.0));
	Serial.print("us per byte, ");
	Serial.print((16.0 * 5000.0 * 1000000.0) / elapsed);
	Serial.println(" bytes per second");

	Serial.println();
}

void setup()
{
	Serial.begin(9600);

	Serial.println();

	Serial.println("State Sizes:");
	Serial.print("Speck ... ");
	Serial.println(sizeof(Speck));
	Serial.print("SpeckSmall ... ");
	Serial.println(sizeof(SpeckSmall));
	Serial.print("SpeckTiny ... ");
	Serial.println(sizeof(SpeckTiny));
	Serial.println();

	Serial.println("Speck Test Vectors:");
	testCipher(&speck, &testVectorSpeck128, 16);
	testCipher(&speck, &testVectorSpeck192, 24);
	testCipher(&speck, &testVectorSpeck256, 32);

	Serial.println();

	Serial.println("SpeckSmall Test Vectors:");
	testCipher(&speckSmall, &testVectorSpeck128, 16);
	testCipher(&speckSmall, &testVectorSpeck192, 24);
	testCipher(&speckSmall, &testVectorSpeck256, 32);

	Serial.println();

	Serial.println("SpeckTiny Test Vectors:");
	testCipher(&speckTiny, &testVectorSpeck128, 16, false);
	testCipher(&speckTiny, &testVectorSpeck192, 24, false);
	testCipher(&speckTiny, &testVectorSpeck256, 32, false);

	Serial.println();

	Serial.println("Speck Performance Tests:");
	perfCipher(&speck, &testVectorSpeck128, 16);
	perfCipher(&speck, &testVectorSpeck192, 24);
	perfCipher(&speck, &testVectorSpeck256, 32);

	Serial.println("SpeckSmall Performance Tests:");
	perfCipher(&speckSmall, &testVectorSpeck128, 16);
	perfCipher(&speckSmall, &testVectorSpeck192, 24);
	perfCipher(&speckSmall, &testVectorSpeck256, 32);

	Serial.println("SpeckTiny Performance Tests:");
	perfCipher(&speckTiny, &testVectorSpeck128, 16, false);
	perfCipher(&speckTiny, &testVectorSpeck192, 24, false);
	perfCipher(&speckTiny, &testVectorSpeck256, 32, false);
}

void loop()
{
}
