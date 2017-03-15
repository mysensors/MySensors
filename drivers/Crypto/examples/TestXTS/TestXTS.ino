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

/*
This example runs tests on the XTS implementation to verify correct behaviour.
*/

#include <Crypto.h>
#include <AES.h>
#include <Speck.h>
#include <SpeckSmall.h>
#include <SpeckTiny.h>
#include <XTS.h>
#include <string.h>
#include <avr/pgmspace.h>

#define MAX_SECTOR_SIZE 64

struct TestVector {
	const char *name;
	byte key1[16];
	byte key2[16];
	byte plaintext[MAX_SECTOR_SIZE];
	byte ciphertext[MAX_SECTOR_SIZE];
	byte tweak[16];
	size_t sectorSize;
};

// Selected test vectors for XTS-AES-128 from:
// http://libeccio.di.unisa.it/Crypto14/Lab/p1619.pdf
static TestVector const testVectorXTSAES128_1 PROGMEM = {
	.name        = "XTS-AES-128 #1",
	.key1        = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	.key2        = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	.plaintext   = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},
	.ciphertext  = {
		0x91, 0x7c, 0xf6, 0x9e, 0xbd, 0x68, 0xb2, 0xec,
		0x9b, 0x9f, 0xe9, 0xa3, 0xea, 0xdd, 0xa6, 0x92,
		0xcd, 0x43, 0xd2, 0xf5, 0x95, 0x98, 0xed, 0x85,
		0x8c, 0x02, 0xc2, 0x65, 0x2f, 0xbf, 0x92, 0x2e
	},
	.tweak       = {0x00},
	.sectorSize  = 32
};
static TestVector const testVectorXTSAES128_2 PROGMEM = {
	.name        = "XTS-AES-128 #2",
	.key1        = {
		0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
		0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
	},
	.key2        = {
		0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
		0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22
	},
	.plaintext   = {
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44
	},
	.ciphertext  = {
		0xc4, 0x54, 0x18, 0x5e, 0x6a, 0x16, 0x93, 0x6e,
		0x39, 0x33, 0x40, 0x38, 0xac, 0xef, 0x83, 0x8b,
		0xfb, 0x18, 0x6f, 0xff, 0x74, 0x80, 0xad, 0xc4,
		0x28, 0x93, 0x82, 0xec, 0xd6, 0xd3, 0x94, 0xf0
	},
	.tweak       = {0x33, 0x33, 0x33, 0x33, 0x33},
	.sectorSize  = 32
};
static TestVector const testVectorXTSAES128_3 PROGMEM = {
	.name        = "XTS-AES-128 #3",
	.key1        = {
		0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
		0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0
	},
	.key2        = {
		0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22,
		0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22
	},
	.plaintext   = {
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44,
		0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44
	},
	.ciphertext  = {
		0xaf, 0x85, 0x33, 0x6b, 0x59, 0x7a, 0xfc, 0x1a,
		0x90, 0x0b, 0x2e, 0xb2, 0x1e, 0xc9, 0x49, 0xd2,
		0x92, 0xdf, 0x4c, 0x04, 0x7e, 0x0b, 0x21, 0x53,
		0x21, 0x86, 0xa5, 0x97, 0x1a, 0x22, 0x7a, 0x89
	},
	.tweak       = {0x33, 0x33, 0x33, 0x33, 0x33},
	.sectorSize  = 32
};
static TestVector const testVectorXTSAES128_4 PROGMEM = {
	// 512 byte test vector from the spec truncated to the first 64 bytes.
	.name        = "XTS-AES-128 #4",
	.key1        = {
		0x27, 0x18, 0x28, 0x18, 0x28, 0x45, 0x90, 0x45,
		0x23, 0x53, 0x60, 0x28, 0x74, 0x71, 0x35, 0x26
	},
	.key2        = {
		0x31, 0x41, 0x59, 0x26, 0x53, 0x58, 0x97, 0x93,
		0x23, 0x84, 0x62, 0x64, 0x33, 0x83, 0x27, 0x95
	},
	.plaintext   = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
		0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f
	},
	.ciphertext  = {
		0x27, 0xa7, 0x47, 0x9b, 0xef, 0xa1, 0xd4, 0x76,
		0x48, 0x9f, 0x30, 0x8c, 0xd4, 0xcf, 0xa6, 0xe2,
		0xa9, 0x6e, 0x4b, 0xbe, 0x32, 0x08, 0xff, 0x25,
		0x28, 0x7d, 0xd3, 0x81, 0x96, 0x16, 0xe8, 0x9c,
		0xc7, 0x8c, 0xf7, 0xf5, 0xe5, 0x43, 0x44, 0x5f,
		0x83, 0x33, 0xd8, 0xfa, 0x7f, 0x56, 0x00, 0x00,
		0x05, 0x27, 0x9f, 0xa5, 0xd8, 0xb5, 0xe4, 0xad,
		0x40, 0xe7, 0x36, 0xdd, 0xb4, 0xd3, 0x54, 0x12
	},
	.tweak       = {0x00},
	.sectorSize  = 64
};
static TestVector const testVectorXTSAES128_15 PROGMEM = {
	.name        = "XTS-AES-128 #15",
	.key1        = {
		0xff, 0xfe, 0xfd, 0xfc, 0xfb, 0xfa, 0xf9, 0xf8,
		0xf7, 0xf6, 0xf5, 0xf4, 0xf3, 0xf2, 0xf1, 0xf0
	},
	.key2        = {
		0xbf, 0xbe, 0xbd, 0xbc, 0xbb, 0xba, 0xb9, 0xb8,
		0xb7, 0xb6, 0xb5, 0xb4, 0xb3, 0xb2, 0xb1, 0xb0
	},
	.plaintext   = {
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10
	},
	.ciphertext  = {
		0x6c, 0x16, 0x25, 0xdb, 0x46, 0x71, 0x52, 0x2d,
		0x3d, 0x75, 0x99, 0x60, 0x1d, 0xe7, 0xca, 0x09,
		0xed
	},
	.tweak       = {0x9a, 0x78, 0x56, 0x34, 0x12},
	.sectorSize  = 17
};
// This test vector is from:
// https://github.com/heisencoder/XTS-AES/blob/master/testvals/xts.4
// We use this one because the main specification doesn't have an odd
// block size greater than 32 bytes but less than 64 bytes.
static TestVector const testVectorXTSAES128_16 PROGMEM = {
	.name        = "XTS-AES-128 #16",
	.key1        = {
		0x27, 0x18, 0x28, 0x18, 0x28, 0x45, 0x90, 0x45,
		0x23, 0x53, 0x60, 0x28, 0x74, 0x71, 0x35, 0x26
	},
	.key2        = {
		0x31, 0x41, 0x59, 0x26, 0x53, 0x58, 0x97, 0x93,
		0x23, 0x84, 0x62, 0x64, 0x33, 0x83, 0x27, 0x95
	},
	.plaintext   = {
		0x50, 0x00, 0xec, 0xa5, 0xa1, 0xf6, 0xa4, 0x93,
		0x78, 0x03, 0x0d, 0x9e, 0xe8, 0x05, 0xac, 0xef,
		0x46, 0x0f, 0x31, 0x4e, 0xe0, 0x4b, 0xb5, 0x14,
		0x03, 0x4e, 0xb2, 0x7f, 0xb8, 0xdf, 0x2b, 0xc8,
		0x12, 0xae, 0x5b, 0xdf, 0x8c
	},
	.ciphertext  = {
		0xe5, 0x9e, 0x6f, 0x23, 0x3b, 0xe0, 0xe0, 0x83,
		0x04, 0x83, 0xc6, 0xbd, 0x4e, 0x82, 0xf4, 0xc3,
		0x95, 0x43, 0x55, 0x8a, 0x25, 0xe3, 0xdb, 0x60,
		0xa5, 0x53, 0xa5, 0x94, 0x81, 0x45, 0xa6, 0xff,
		0xb5, 0xe6, 0xbe, 0x1d, 0xb5
	},
	.tweak       = {0x33, 0x22, 0x11, 0x00},
	.sectorSize  = 37
};

XTS<AES128> *xtsaes128;
TestVector testVector;

byte buffer[MAX_SECTOR_SIZE];

#if defined(__AVR__)

void _printProgMem(const char *str)
{
	for (;;) {
		uint8_t ch = pgm_read_byte((const uint8_t *)str);
		if (!ch) {
			break;
		}
		Serial.write(ch);
		++str;
	}
}

#define printProgMem(str) \
	do { \
		static char const temp_str[] PROGMEM = str; \
		_printProgMem(temp_str); \
	} while (0)

#define printlnProgMem(str) \
	do { \
		static char const temp_str[] PROGMEM = str; \
		_printProgMem(temp_str); \
		Serial.println(); \
	} while (0)

#else

#define printProgMem(str) \
	Serial.print(str)

#define printlnProgMem(str) \
	Serial.println(str)

#endif

void testXTS(XTSCommon *cipher, const struct TestVector *test)
{
	memcpy_P(&testVector, test, sizeof(testVector));

	Serial.print(testVector.name);
	printProgMem(" Encrypt ... ");

	cipher->setSectorSize(testVector.sectorSize);
	cipher->setKey(testVector.key1, 32);
	cipher->setTweak(testVector.tweak, sizeof(testVector.tweak));
	cipher->encryptSector(buffer, testVector.plaintext);

	if (!memcmp(buffer, testVector.ciphertext, testVector.sectorSize)) {
		printlnProgMem("Passed");
	} else {
		printlnProgMem("Failed");
	}

	Serial.print(testVector.name);
	printProgMem(" Decrypt ... ");

	cipher->decryptSector(buffer, testVector.ciphertext);

	if (!memcmp(buffer, testVector.plaintext, testVector.sectorSize)) {
		printlnProgMem("Passed");
	} else {
		printlnProgMem("Failed");
	}

	Serial.print(testVector.name);
	printProgMem(" Encrypt In-Place ... ");

	memcpy(buffer, testVector.plaintext, testVector.sectorSize);
	cipher->encryptSector(buffer, buffer);

	if (!memcmp(buffer, testVector.ciphertext, testVector.sectorSize)) {
		printlnProgMem("Passed");
	} else {
		printlnProgMem("Failed");
	}

	Serial.print(testVector.name);
	printProgMem(" Decrypt In-Place ... ");

	memcpy(buffer, testVector.ciphertext, testVector.sectorSize);
	cipher->decryptSector(buffer, buffer);

	if (!memcmp(buffer, testVector.plaintext, testVector.sectorSize)) {
		printlnProgMem("Passed");
	} else {
		printlnProgMem("Failed");
	}
}

void perfEncrypt(const char *name, XTSCommon *cipher, const struct TestVector *test,
                 size_t keySize = 32)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(testVector));

	Serial.print(name);
	printProgMem(" ... ");

	cipher->setSectorSize(sizeof(buffer));
	cipher->setKey(testVector.key1, keySize);
	cipher->setTweak(testVector.tweak, sizeof(testVector.tweak));
	memset(buffer, 0xAA, sizeof(buffer));
	start = micros();
	for (count = 0; count < 500; ++count) {
		cipher->encryptSector(buffer, buffer);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (sizeof(buffer) * 500.0));
	printProgMem("us per byte, ");
	Serial.print((sizeof(buffer) * 500.0 * 1000000.0) / elapsed);
	printlnProgMem(" bytes per second");
}

void perfDecrypt(const char *name, XTSCommon *cipher, const struct TestVector *test,
                 size_t keySize = 32)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(testVector));

	Serial.print(name);
	printProgMem(" ... ");

	cipher->setSectorSize(sizeof(buffer));
	cipher->setKey(testVector.key1, keySize);
	cipher->setTweak(testVector.tweak, sizeof(testVector.tweak));
	start = micros();
	for (count = 0; count < 500; ++count) {
		cipher->decryptSector(buffer, buffer);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / (sizeof(buffer) * 500.0));
	printProgMem("us per byte, ");
	Serial.print((sizeof(buffer) * 500.0 * 1000000.0) / elapsed);
	printlnProgMem(" bytes per second");
}

void perfSetKey(const char *name, XTSCommon *cipher, const struct TestVector *test,
                size_t keySize = 32)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(testVector));

	Serial.print(name);
	printProgMem(" ... ");

	start = micros();
	for (count = 0; count < 2000; ++count) {
		cipher->setKey(testVector.key1, keySize);
	}
	elapsed = micros() - start;

	Serial.print(elapsed / 2000.0);
	printProgMem("us per operation, ");
	Serial.print((2000.0 * 1000000.0) / elapsed);
	printlnProgMem(" operations per second");
}

void perfSetTweak(const char *name, XTSCommon *cipher, const struct TestVector *test)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	memcpy_P(&testVector, test, sizeof(testVector));

	Serial.print(name);
	printProgMem(" ... ");

	start = micros();
	for (count = 0; count < 2000; ++count) {
		cipher->setTweak(testVector.tweak, sizeof(testVector.tweak));
	}
	elapsed = micros() - start;

	Serial.print(elapsed / 2000.0);
	printProgMem("us per operation, ");
	Serial.print((2000.0 * 1000000.0) / elapsed);
	printlnProgMem(" operations per second");
}

void setup()
{
	Serial.begin(9600);

	Serial.println();

	xtsaes128 = new XTS<AES128>();
	printlnProgMem("State Sizes:");
	printProgMem("XTS<AES128> ... ");
	Serial.println(sizeof(*xtsaes128));
	printProgMem("XTS<AES256> ... ");
	Serial.println(sizeof(XTS<AES256>));
	printProgMem("XTS<Speck> ... ");
	Serial.println(sizeof(XTS<Speck>));
	printProgMem("XTS<SpeckSmall> ... ");
	Serial.println(sizeof(XTS<SpeckSmall>));
	printProgMem("XTS<SpeckSmall, SpeckTiny> ... ");
	Serial.println(sizeof(XTS<SpeckSmall, SpeckTiny>));

	printProgMem("XTSSingleKey<AES128> ... ");
	Serial.println(sizeof(XTSSingleKey<AES128>));
	printProgMem("XTSSingleKey<AES256> ... ");
	Serial.println(sizeof(XTSSingleKey<AES256>));
	printProgMem("XTSSingleKey<Speck> ... ");
	Serial.println(sizeof(XTSSingleKey<Speck>));
	printProgMem("XTSSingleKey<SpeckSmall> ... ");
	Serial.println(sizeof(XTSSingleKey<SpeckSmall>));

	Serial.println();

	printlnProgMem("Test Vectors:");
	testXTS(xtsaes128, &testVectorXTSAES128_1);
	testXTS(xtsaes128, &testVectorXTSAES128_2);
	testXTS(xtsaes128, &testVectorXTSAES128_3);
	testXTS(xtsaes128, &testVectorXTSAES128_4);
	testXTS(xtsaes128, &testVectorXTSAES128_15);
	testXTS(xtsaes128, &testVectorXTSAES128_16);

	Serial.println();

	printlnProgMem("Performance Tests:");
	Serial.println();

	printlnProgMem("XTS-AES-128:");
	perfEncrypt("Encrypt", xtsaes128, &testVectorXTSAES128_4);
	perfDecrypt("Decrypt", xtsaes128, &testVectorXTSAES128_4);
	perfSetKey("Set Key", xtsaes128, &testVectorXTSAES128_4);
	perfSetTweak("Set Tweak", xtsaes128, &testVectorXTSAES128_4);
	delete xtsaes128;
	Serial.println();

	printlnProgMem("XTS-AES-128 Single Key:");
	XTSSingleKey<AES128> *singleaes128 = new XTSSingleKey<AES128>();
	perfEncrypt("Encrypt", singleaes128, &testVectorXTSAES128_4, 16);
	perfDecrypt("Decrypt", singleaes128, &testVectorXTSAES128_4, 16);
	perfSetKey("Set Key", singleaes128, &testVectorXTSAES128_4, 16);
	perfSetTweak("Set Tweak", singleaes128, &testVectorXTSAES128_4);
	delete singleaes128;
	Serial.println();

	printlnProgMem("XTS-AES-256 Single Key:");
	XTSSingleKey<AES256> *xtsaes256 = new XTSSingleKey<AES256>();
	perfEncrypt("Encrypt", xtsaes256, &testVectorXTSAES128_4, 32);
	perfDecrypt("Decrypt", xtsaes256, &testVectorXTSAES128_4, 32);
	perfSetKey("Set Key", xtsaes256, &testVectorXTSAES128_4, 32);
	perfSetTweak("Set Tweak", xtsaes256, &testVectorXTSAES128_4);
	delete xtsaes256;
	Serial.println();

	printlnProgMem("XTS-SpeckSmall-256:");
	XTS<SpeckSmall, SpeckTiny> *xtsspeck = new XTS<SpeckSmall, SpeckTiny>();
	perfEncrypt("Encrypt", xtsspeck, &testVectorXTSAES128_4, 64);
	perfDecrypt("Decrypt", xtsspeck, &testVectorXTSAES128_4, 64);
	perfSetKey("Set Key", xtsspeck, &testVectorXTSAES128_4, 64);
	perfSetTweak("Set Tweak", xtsspeck, &testVectorXTSAES128_4);
	delete xtsspeck;
	Serial.println();

	printlnProgMem("XTS-SpeckSmall-256 Single Key:");
	XTSSingleKey<SpeckSmall> *singlespeck = new XTSSingleKey<SpeckSmall>();
	perfEncrypt("Encrypt", singlespeck, &testVectorXTSAES128_4, 32);
	perfDecrypt("Decrypt", singlespeck, &testVectorXTSAES128_4, 32);
	perfSetKey("Set Key", singlespeck, &testVectorXTSAES128_4, 32);
	perfSetTweak("Set Tweak", singlespeck, &testVectorXTSAES128_4);
	delete singlespeck;
	Serial.println();

	printlnProgMem("XTS-Speck-256:");
	XTS<Speck> *xtsspeck2 = new XTS<Speck>();
	perfEncrypt("Encrypt", xtsspeck2, &testVectorXTSAES128_4, 64);
	perfDecrypt("Decrypt", xtsspeck2, &testVectorXTSAES128_4, 64);
	perfSetKey("Set Key", xtsspeck2, &testVectorXTSAES128_4, 64);
	perfSetTweak("Set Tweak", xtsspeck2, &testVectorXTSAES128_4);
	delete xtsspeck2;
	Serial.println();

	printlnProgMem("XTS-Speck-256 Single Key:");
	XTSSingleKey<Speck> *singlespeck2 = new XTSSingleKey<Speck>();
	perfEncrypt("Encrypt", singlespeck2, &testVectorXTSAES128_4, 32);
	perfDecrypt("Decrypt", singlespeck2, &testVectorXTSAES128_4, 32);
	perfSetKey("Set Key", singlespeck2, &testVectorXTSAES128_4, 32);
	perfSetTweak("Set Tweak", singlespeck2, &testVectorXTSAES128_4);
	delete singlespeck2;
	Serial.println();
}

void loop()
{
}
