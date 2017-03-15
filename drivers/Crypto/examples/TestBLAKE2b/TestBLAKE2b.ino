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
This example runs tests on the BLAKE2b implementation to verify correct behaviour.
*/

#include <Crypto.h>
#include <BLAKE2b.h>
#include <string.h>
#include <avr/pgmspace.h>

#define HASH_SIZE 64
#define BLOCK_SIZE 128

struct TestHashVector {
	const char *name;
	const char *data;
	uint8_t hash[HASH_SIZE];
};

// Test vectors generated with the reference implementation of BLAKE2b.
static TestHashVector const testVectorBLAKE2b_1 PROGMEM = {
	"BLAKE2b #1",
	"",
	{
		0x78, 0x6a, 0x02, 0xf7, 0x42, 0x01, 0x59, 0x03,
		0xc6, 0xc6, 0xfd, 0x85, 0x25, 0x52, 0xd2, 0x72,
		0x91, 0x2f, 0x47, 0x40, 0xe1, 0x58, 0x47, 0x61,
		0x8a, 0x86, 0xe2, 0x17, 0xf7, 0x1f, 0x54, 0x19,
		0xd2, 0x5e, 0x10, 0x31, 0xaf, 0xee, 0x58, 0x53,
		0x13, 0x89, 0x64, 0x44, 0x93, 0x4e, 0xb0, 0x4b,
		0x90, 0x3a, 0x68, 0x5b, 0x14, 0x48, 0xb7, 0x55,
		0xd5, 0x6f, 0x70, 0x1a, 0xfe, 0x9b, 0xe2, 0xce
	}
};
static TestHashVector const testVectorBLAKE2b_2 PROGMEM = {
	"BLAKE2b #2",
	"abc",
	{
		0xba, 0x80, 0xa5, 0x3f, 0x98, 0x1c, 0x4d, 0x0d,
		0x6a, 0x27, 0x97, 0xb6, 0x9f, 0x12, 0xf6, 0xe9,
		0x4c, 0x21, 0x2f, 0x14, 0x68, 0x5a, 0xc4, 0xb7,
		0x4b, 0x12, 0xbb, 0x6f, 0xdb, 0xff, 0xa2, 0xd1,
		0x7d, 0x87, 0xc5, 0x39, 0x2a, 0xab, 0x79, 0x2d,
		0xc2, 0x52, 0xd5, 0xde, 0x45, 0x33, 0xcc, 0x95,
		0x18, 0xd3, 0x8a, 0xa8, 0xdb, 0xf1, 0x92, 0x5a,
		0xb9, 0x23, 0x86, 0xed, 0xd4, 0x00, 0x99, 0x23
	}
};
static TestHashVector const testVectorBLAKE2b_3 PROGMEM = {
	"BLAKE2b #3",
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	{
		0x72, 0x85, 0xff, 0x3e, 0x8b, 0xd7, 0x68, 0xd6,
		0x9b, 0xe6, 0x2b, 0x3b, 0xf1, 0x87, 0x65, 0xa3,
		0x25, 0x91, 0x7f, 0xa9, 0x74, 0x4a, 0xc2, 0xf5,
		0x82, 0xa2, 0x08, 0x50, 0xbc, 0x2b, 0x11, 0x41,
		0xed, 0x1b, 0x3e, 0x45, 0x28, 0x59, 0x5a, 0xcc,
		0x90, 0x77, 0x2b, 0xdf, 0x2d, 0x37, 0xdc, 0x8a,
		0x47, 0x13, 0x0b, 0x44, 0xf3, 0x3a, 0x02, 0xe8,
		0x73, 0x0e, 0x5a, 0xd8, 0xe1, 0x66, 0xe8, 0x88
	}
};
static TestHashVector const testVectorBLAKE2b_4 PROGMEM = {
	"BLAKE2b #4",
	"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
	"hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
	{
		0xce, 0x74, 0x1a, 0xc5, 0x93, 0x0f, 0xe3, 0x46,
		0x81, 0x11, 0x75, 0xc5, 0x22, 0x7b, 0xb7, 0xbf,
		0xcd, 0x47, 0xf4, 0x26, 0x12, 0xfa, 0xe4, 0x6c,
		0x08, 0x09, 0x51, 0x4f, 0x9e, 0x0e, 0x3a, 0x11,
		0xee, 0x17, 0x73, 0x28, 0x71, 0x47, 0xcd, 0xea,
		0xee, 0xdf, 0xf5, 0x07, 0x09, 0xaa, 0x71, 0x63,
		0x41, 0xfe, 0x65, 0x24, 0x0f, 0x4a, 0xd6, 0x77,
		0x7d, 0x6b, 0xfa, 0xf9, 0x72, 0x6e, 0x5e, 0x52
	}
};

BLAKE2b blake2b;

byte buffer[BLOCK_SIZE + 2];

bool testHash_N(Hash *hash, const struct TestHashVector *test, size_t inc)
{
	size_t size = strlen(test->data);
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
	TestHashVector vec;

	memcpy_P(&vec, test, sizeof(vec));
	test = &vec;

	Serial.print(test->name);
	Serial.print(" ... ");

	ok  = testHash_N(hash, test, strlen(test->data));
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

	Serial.print("Hashing ... ");

	for (size_t posn = 0; posn < sizeof(buffer); ++posn) {
		buffer[posn] = (uint8_t)posn;
	}

	hash->reset();
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

	Serial.print("HMAC-BLAKE2b keysize=");
	Serial.print(keyLen);
	Serial.print(" ... ");

	// Construct the expected result with a simple HMAC implementation.
	memset(buffer, (uint8_t)keyLen, keyLen);
	hashKey(hash, buffer, keyLen, 0x36);
	memset(buffer, 0xBA, sizeof(buffer));
	hash->update(buffer, sizeof(buffer));
	hash->finalize(result, HASH_SIZE);
	memset(buffer, (uint8_t)keyLen, keyLen);
	hashKey(hash, buffer, keyLen, 0x5C);
	hash->update(result, HASH_SIZE);
	hash->finalize(result, HASH_SIZE);

	// Now use the library to compute the HMAC.
	hash->resetHMAC(buffer, keyLen);
	memset(buffer, 0xBA, sizeof(buffer));
	hash->update(buffer, sizeof(buffer));
	memset(buffer, (uint8_t)keyLen, keyLen);
	hash->finalizeHMAC(buffer, keyLen, buffer, HASH_SIZE);

	// Check the result.
	if (!memcmp(result, buffer, HASH_SIZE)) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
	}
}

// Deterministic sequences (Fibonacci generator).  From RFC 7693.
static void selftest_seq(uint8_t *out, size_t len, uint32_t seed)
{
	size_t i;
	uint32_t t, a , b;

	a = 0xDEAD4BAD * seed;              // prime
	b = 1;

	for (i = 0; i < len; i++) {         // fill the buf
		t = a + b;
		a = b;
		b = t;
		out[i] = (t >> 24) & 0xFF;
	}
}

// Incremental version of above to save memory.
static void selftest_seq_incremental(BLAKE2b *blake, size_t len, uint32_t seed)
{
	size_t i;
	uint32_t t, a , b;

	a = 0xDEAD4BAD * seed;              // prime
	b = 1;

	for (i = 0; i < len; i++) {         // fill the buf
		t = a + b;
		a = b;
		b = t;
		buffer[i % 128] = (t >> 24) & 0xFF;
		if ((i % 128) == 127) {
			blake->update(buffer, 128);
		}
	}

	blake->update(buffer, len % 128);
}

// Run the self-test from Appendix E of RFC 7693.  Most of this code
// is from RFC 7693, with modifications to use the Crypto library.
void testRFC7693()
{
	// Grand hash of hash results.
	static const uint8_t blake2b_res[32] PROGMEM = {
		0xC2, 0x3A, 0x78, 0x00, 0xD9, 0x81, 0x23, 0xBD,
		0x10, 0xF5, 0x06, 0xC6, 0x1E, 0x29, 0xDA, 0x56,
		0x03, 0xD7, 0x63, 0xB8, 0xBB, 0xAD, 0x2E, 0x73,
		0x7F, 0x5E, 0x76, 0x5A, 0x7B, 0xCC, 0xD4, 0x75
	};
	// Parameter sets.
	static const uint8_t b2b_md_len[4] PROGMEM = { 20, 32, 48, 64 };
	static const uint16_t b2b_in_len[6] PROGMEM = { 0, 3, 128, 129, 255, 1024 };

	size_t i, j, outlen, inlen;
	uint8_t md[64], key[64];
	BLAKE2b inner;

	Serial.print("BLAKE2b RFC 7693 ... ");

	// 256-bit hash for testing.
	blake2b.reset(32);

	for (i = 0; i < 4; i++) {
		outlen = pgm_read_byte(&(b2b_md_len[i]));
		for (j = 0; j < 6; j++) {
			inlen = pgm_read_word(&(b2b_in_len[j]));

			inner.reset(outlen);                // unkeyed hash
			selftest_seq_incremental(&inner, inlen, inlen);
			inner.finalize(md, outlen);
			blake2b.update(md, outlen);         // hash the hash

			selftest_seq(key, outlen, outlen);  // keyed hash
			inner.reset(key, outlen, outlen);
			selftest_seq_incremental(&inner, inlen, inlen);
			inner.finalize(md, outlen);
			blake2b.update(md, outlen);         // hash the hash
		}
	}

	// Compute and compare the hash of hashes.
	bool ok = true;
	blake2b.finalize(md, 32);
	for (i = 0; i < 32; i++) {
		if (md[i] != pgm_read_byte(&(blake2b_res[i]))) {
			ok = false;
		}
	}

	// Report the results.
	if (ok) {
		Serial.println("Passed");
	} else {
		Serial.println("Failed");
	}
}

void perfKeyed(BLAKE2b *hash)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print("Keyed Reset ... ");

	for (size_t posn = 0; posn < sizeof(buffer); ++posn) {
		buffer[posn] = (uint8_t)posn;
	}

	start = micros();
	for (count = 0; count < 1000; ++count) {
		hash->reset(buffer, hash->hashSize());
		hash->update(buffer, 1);    // To flush the key chunk.
	}
	elapsed = micros() - start;

	Serial.print(elapsed / 1000.0);
	Serial.print("us per op, ");
	Serial.print((1000.0 * 1000000.0) / elapsed);
	Serial.println(" ops per second");
}

void perfFinalize(Hash *hash)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

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
	Serial.println(sizeof(BLAKE2b));
	Serial.println();

	Serial.println("Test Vectors:");
	testHash(&blake2b, &testVectorBLAKE2b_1);
	testHash(&blake2b, &testVectorBLAKE2b_2);
	testHash(&blake2b, &testVectorBLAKE2b_3);
	testHash(&blake2b, &testVectorBLAKE2b_4);
	testHMAC(&blake2b, (size_t)0);
	testHMAC(&blake2b, 1);
	testHMAC(&blake2b, HASH_SIZE);
	testHMAC(&blake2b, BLOCK_SIZE);
	testHMAC(&blake2b, BLOCK_SIZE + 1);
	testHMAC(&blake2b, BLOCK_SIZE + 2);
	testRFC7693();

	Serial.println();

	Serial.println("Performance Tests:");
	perfHash(&blake2b);
	perfKeyed(&blake2b);
	perfFinalize(&blake2b);
}

void loop()
{
}
