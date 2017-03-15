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
This example runs tests on the BLAKE2s implementation to verify correct behaviour.
*/

#include <Crypto.h>
#include <BLAKE2s.h>
#include <string.h>
#include <avr/pgmspace.h>

#define HASH_SIZE 32
#define BLOCK_SIZE 64

struct TestHashVector {
	const char *name;
	const char *data;
	uint8_t hash[HASH_SIZE];
};

// Test vectors generated with the reference implementation of BLAKE2s.
static TestHashVector const testVectorBLAKE2s_1 = {
	"BLAKE2s #1",
	"",
	{
		0x69, 0x21, 0x7a, 0x30, 0x79, 0x90, 0x80, 0x94,
		0xe1, 0x11, 0x21, 0xd0, 0x42, 0x35, 0x4a, 0x7c,
		0x1f, 0x55, 0xb6, 0x48, 0x2c, 0xa1, 0xa5, 0x1e,
		0x1b, 0x25, 0x0d, 0xfd, 0x1e, 0xd0, 0xee, 0xf9
	}
};
static TestHashVector const testVectorBLAKE2s_2 = {
	"BLAKE2s #2",
	"abc",
	{
		0x50, 0x8c, 0x5e, 0x8c, 0x32, 0x7c, 0x14, 0xe2,
		0xe1, 0xa7, 0x2b, 0xa3, 0x4e, 0xeb, 0x45, 0x2f,
		0x37, 0x45, 0x8b, 0x20, 0x9e, 0xd6, 0x3a, 0x29,
		0x4d, 0x99, 0x9b, 0x4c, 0x86, 0x67, 0x59, 0x82
	}
};
static TestHashVector const testVectorBLAKE2s_3 = {
	"BLAKE2s #3",
	"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
	{
		0x6f, 0x4d, 0xf5, 0x11, 0x6a, 0x6f, 0x33, 0x2e,
		0xda, 0xb1, 0xd9, 0xe1, 0x0e, 0xe8, 0x7d, 0xf6,
		0x55, 0x7b, 0xea, 0xb6, 0x25, 0x9d, 0x76, 0x63,
		0xf3, 0xbc, 0xd5, 0x72, 0x2c, 0x13, 0xf1, 0x89
	}
};
static TestHashVector const testVectorBLAKE2s_4 = {
	"BLAKE2s #4",
	"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
	"hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
	{
		0x35, 0x8d, 0xd2, 0xed, 0x07, 0x80, 0xd4, 0x05,
		0x4e, 0x76, 0xcb, 0x6f, 0x3a, 0x5b, 0xce, 0x28,
		0x41, 0xe8, 0xe2, 0xf5, 0x47, 0x43, 0x1d, 0x4d,
		0x09, 0xdb, 0x21, 0xb6, 0x6d, 0x94, 0x1f, 0xc7
	}
};

BLAKE2s blake2s;

byte buffer[128];

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

	Serial.print("HMAC-BLAKE2s keysize=");
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
static void selftest_seq_incremental(BLAKE2s *blake, size_t len, uint32_t seed)
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
			blake->update(buffer, sizeof(buffer));
		}
	}

	blake->update(buffer, len % 128);
}

// Run the self-test from Appendix E of RFC 7693.  Most of this code
// is from RFC 7693, with modifications to use the Crypto library.
void testRFC7693()
{
	// Grand hash of hash results.
	static const uint8_t blake2s_res[32] PROGMEM = {
		0x6A, 0x41, 0x1F, 0x08, 0xCE, 0x25, 0xAD, 0xCD,
		0xFB, 0x02, 0xAB, 0xA6, 0x41, 0x45, 0x1C, 0xEC,
		0x53, 0xC5, 0x98, 0xB2, 0x4F, 0x4F, 0xC7, 0x87,
		0xFB, 0xDC, 0x88, 0x79, 0x7F, 0x4C, 0x1D, 0xFE
	};
	// Parameter sets.
	static const uint8_t b2s_md_len[4] PROGMEM = { 16, 20, 28, 32 };
	static const uint16_t b2s_in_len[6] PROGMEM = { 0,  3,  64, 65, 255, 1024 };

	size_t i, j, outlen, inlen;
	uint8_t md[32], key[32];
	BLAKE2s inner;

	Serial.print("BLAKE2s RFC 7693 ... ");

	// 256-bit hash for testing.
	blake2s.reset(32);

	for (i = 0; i < 4; i++) {
		outlen = pgm_read_byte(&(b2s_md_len[i]));
		for (j = 0; j < 6; j++) {
			inlen = pgm_read_word(&(b2s_in_len[j]));

			inner.reset(outlen);                // unkeyed hash
			selftest_seq_incremental(&inner, inlen, inlen);
			inner.finalize(md, outlen);
			blake2s.update(md, outlen);         // hash the hash

			selftest_seq(key, outlen, outlen);  // keyed hash
			inner.reset(key, outlen, outlen);
			selftest_seq_incremental(&inner, inlen, inlen);
			inner.finalize(md, outlen);
			blake2s.update(md, outlen);         // hash the hash
		}
	}

	// Compute and compare the hash of hashes.
	bool ok = true;
	blake2s.finalize(md, 32);
	for (i = 0; i < 32; i++) {
		if (md[i] != pgm_read_byte(&(blake2s_res[i]))) {
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

void perfKeyed(BLAKE2s *hash)
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

void perfHMAC(Hash *hash)
{
	unsigned long start;
	unsigned long elapsed;
	int count;

	Serial.print("HMAC Reset ... ");

	for (size_t posn = 0; posn < sizeof(buffer); ++posn) {
		buffer[posn] = (uint8_t)posn;
	}

	start = micros();
	for (count = 0; count < 1000; ++count) {
		hash->resetHMAC(buffer, hash->hashSize());
	}
	elapsed = micros() - start;

	Serial.print(elapsed / 1000.0);
	Serial.print("us per op, ");
	Serial.print((1000.0 * 1000000.0) / elapsed);
	Serial.println(" ops per second");

	Serial.print("HMAC Finalize ... ");

	hash->resetHMAC(buffer, hash->hashSize());
	hash->update("abc", 3);
	start = micros();
	for (count = 0; count < 1000; ++count) {
		hash->finalizeHMAC(buffer, hash->hashSize(), buffer, hash->hashSize());
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

	Serial.print("State Size ... ");
	Serial.println(sizeof(BLAKE2s));
	Serial.println();

	Serial.println("Test Vectors:");
	testHash(&blake2s, &testVectorBLAKE2s_1);
	testHash(&blake2s, &testVectorBLAKE2s_2);
	testHash(&blake2s, &testVectorBLAKE2s_3);
	testHash(&blake2s, &testVectorBLAKE2s_4);
	testHMAC(&blake2s, (size_t)0);
	testHMAC(&blake2s, 1);
	testHMAC(&blake2s, HASH_SIZE);
	testHMAC(&blake2s, BLOCK_SIZE);
	testHMAC(&blake2s, BLOCK_SIZE + 1);
	testHMAC(&blake2s, sizeof(buffer));
	testRFC7693();

	Serial.println();

	Serial.println("Performance Tests:");
	perfHash(&blake2s);
	perfFinalize(&blake2s);
	perfKeyed(&blake2s);
	perfHMAC(&blake2s);
}

void loop()
{
}
