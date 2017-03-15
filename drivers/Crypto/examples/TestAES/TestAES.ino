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
This example runs tests on the AES implementation to verify correct behaviour.
*/

#include <Crypto.h>
#include <AES.h>
#include <string.h>

struct TestVector {
	const char *name;
	byte key[32];
	byte plaintext[16];
	byte ciphertext[16];
};

// Define the ECB test vectors from the FIPS specification.
static TestVector const testVectorAES128 = {
	.name        = "AES-128-ECB",
	.key         = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
	},
	.plaintext   = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	},
	.ciphertext  = {
		0x69, 0xC4, 0xE0, 0xD8, 0x6A, 0x7B, 0x04, 0x30,
		0xD8, 0xCD, 0xB7, 0x80, 0x70, 0xB4, 0xC5, 0x5A
	}
};
static TestVector const testVectorAES192 = {
	.name        = "AES-192-ECB",
	.key         = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17
	},
	.plaintext   = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	},
	.ciphertext  = {
		0xDD, 0xA9, 0x7C, 0xA4, 0x86, 0x4C, 0xDF, 0xE0,
		0x6E, 0xAF, 0x70, 0xA0, 0xEC, 0x0D, 0x71, 0x91
	}
};
static TestVector const testVectorAES256 = {
	.name        = "AES-256-ECB",
	.key         = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
	},
	.plaintext   = {
		0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
		0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
	},
	.ciphertext  = {
		0x8E, 0xA2, 0xB7, 0xCA, 0x51, 0x67, 0x45, 0xBF,
		0xEA, 0xFC, 0x49, 0x90, 0x4B, 0x49, 0x60, 0x89
	}
};

AES128 aes128;
AES192 aes192;
AES256 aes256;

byte buffer[16];

void testCipher(BlockCipher *cipher, const struct TestVector *test)
{
	Serial.print(test->name);
	Serial.print(" Encryption ... ");
	cipher->setKey(test->key, cipher->keySize());
	cipher->encryptBlock(buffer, test->plaintext);
	if (memcmp(buffer, test->ciphertext, 16) == 0) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
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

void perfCipher(BlockCipher *cipher, const struct TestVector *test)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print(test->name);
	Serial.print(" Set Key ... ");
	start = micros();
	for (count = 0; count < 10000; ++count) {
		cipher->setKey(test->key, cipher->keySize());
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
	Serial.print("AES128 ... ");
	Serial.println(sizeof(AES128));
	Serial.print("AES192 ... ");
	Serial.println(sizeof(AES192));
	Serial.print("AES256 ... ");
	Serial.println(sizeof(AES256));
	Serial.println();

	Serial.println("Test Vectors:");
	testCipher(&aes128, &testVectorAES128);
	testCipher(&aes192, &testVectorAES192);
	testCipher(&aes256, &testVectorAES256);

	Serial.println();

	Serial.println("Performance Tests:");
	perfCipher(&aes128, &testVectorAES128);
	perfCipher(&aes192, &testVectorAES192);
	perfCipher(&aes256, &testVectorAES256);
}

void loop()
{
}
