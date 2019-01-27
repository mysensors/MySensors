/*
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2019 Sensnology AB
* Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*/

#include "MyCryptoGeneric.h"

const uint32_t SHA256K[] PROGMEM = {
	0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
	0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
	0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
	0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
	0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
	0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
	0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
	0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

const uint8_t SHA256InitState[] PROGMEM = {
	0x67,0xe6,0x09,0x6a, // H0
	0x85,0xae,0x67,0xbb, // H1
	0x72,0xf3,0x6e,0x3c, // H2
	0x3a,0xf5,0x4f,0xa5, // H3
	0x7f,0x52,0x0e,0x51, // H4
	0x8c,0x68,0x05,0x9b, // H5
	0xab,0xd9,0x83,0x1f, // H6
	0x19,0xcd,0xe0,0x5b  // H7
};

_SHA256buffer_t SHA256buffer;
uint8_t SHA256bufferOffset;
_SHA256state_t SHA256state;
uint32_t SHA256byteCount;
uint8_t SHA256keyBuffer[BLOCK_LENGTH];

void SHA256Init(void)
{
	(void)memcpy_P((void *)&SHA256state.b, (const void *)&SHA256InitState, 32);
	SHA256byteCount = 0;
	SHA256bufferOffset = 0;
}

uint32_t SHA256ror32(const uint32_t number, const uint8_t bits)
{
	return ((number << (32 - bits)) | (number >> bits));
}

void SHA256hashBlock(void)
{
	uint32_t a, b, c, d, e, f, g, h, t1, t2;

	a = SHA256state.w[0];
	b = SHA256state.w[1];
	c = SHA256state.w[2];
	d = SHA256state.w[3];
	e = SHA256state.w[4];
	f = SHA256state.w[5];
	g = SHA256state.w[6];
	h = SHA256state.w[7];

	for (uint8_t i = 0; i < 64; i++) {
		if (i >= 16) {
			t1 = SHA256buffer.w[i & 15] + SHA256buffer.w[(i - 7) & 15];
			t2 = SHA256buffer.w[(i - 2) & 15];
			t1 += SHA256ror32(t2, 17) ^ SHA256ror32(t2, 19) ^ (t2 >> 10);
			t2 = SHA256buffer.w[(i - 15) & 15];
			t1 += SHA256ror32(t2, 7) ^ SHA256ror32(t2, 18) ^ (t2 >> 3);
			SHA256buffer.w[i & 15] = t1;
		}
		t1 = h;
		t1 += SHA256ror32(e, 6) ^ SHA256ror32(e, 11) ^ SHA256ror32(e, 25); // ∑1(e)
		t1 += g ^ (e & (g ^ f)); // Ch(e,f,g)
		t1 += pgm_read_dword(SHA256K + i); // Ki
		t1 += SHA256buffer.w[i & 15]; // Wi
		t2 = SHA256ror32(a, 2) ^ SHA256ror32(a, 13) ^ SHA256ror32(a, 22); // ∑0(a)
		t2 += ((b & c) | (a & (b | c))); // Maj(a,b,c)
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}
	SHA256state.w[0] += a;
	SHA256state.w[1] += b;
	SHA256state.w[2] += c;
	SHA256state.w[3] += d;
	SHA256state.w[4] += e;
	SHA256state.w[5] += f;
	SHA256state.w[6] += g;
	SHA256state.w[7] += h;
}

void SHA256addUncounted(const uint8_t data)
{
	SHA256buffer.b[SHA256bufferOffset ^ 3] = data;
	SHA256bufferOffset++;
	if (SHA256bufferOffset == BLOCK_LENGTH) {
		SHA256hashBlock();
		SHA256bufferOffset = 0;
	}
}


void SHA256Add(const uint8_t data)
{
	SHA256byteCount++;
	SHA256addUncounted(data);
}

void SHA256Add(const uint8_t *data, size_t dataLength)
{
	while (dataLength--) {
		SHA256Add(*data++);
	}
}

void SHA256Result(uint8_t *dest)
{
	// Pad to complete the last block
	SHA256addUncounted(0x80);
	while (SHA256bufferOffset != 56) {
		SHA256addUncounted(0x00);
	}

	// Append length in the last 8 bytes
	SHA256addUncounted(0); // We're only using 32 bit lengths
	SHA256addUncounted(0); // But SHA-1 supports 64 bit lengths
	SHA256addUncounted(0); // So zero pad the top bits
	SHA256addUncounted(SHA256byteCount >> 29); // Shifting to multiply by 8
	SHA256addUncounted(SHA256byteCount >> 21); // as SHA-1 supports bitstreams as well as
	SHA256addUncounted(SHA256byteCount >> 13); // byte.
	SHA256addUncounted(SHA256byteCount >> 5);
	SHA256addUncounted(SHA256byteCount << 3);

	// Swap byte order back
	for (uint8_t i = 0; i<8; i++) {
		uint32_t a, b;
		a = SHA256state.w[i];
		b = a << 24;
		b |= (a << 8) & 0x00ff0000;
		b |= (a >> 8) & 0x0000ff00;
		b |= a >> 24;
		SHA256state.w[i] = b;
	}
	(void)memcpy((void *)dest, (const void *)SHA256state.b, 32);
	// Return pointer to hash (20 characters)
	//return SHA256state.b;
}

void SHA256(uint8_t *dest, const uint8_t *data, size_t dataLength)
{
	SHA256Init();
	SHA256Add(data, dataLength);
	SHA256Result(dest);
}

// SHA256HMAC

void SHA256HMACInit(const uint8_t *key, size_t keyLength)
{
	(void)memset((void *)&SHA256keyBuffer, 0x00, BLOCK_LENGTH);
	if (keyLength > BLOCK_LENGTH) {
		// Hash long keys
		SHA256Init();
		SHA256Add(key, keyLength);
		SHA256Result(SHA256keyBuffer);
	} else {
		// Block length keys are used as is
		(void)memcpy((void *)SHA256keyBuffer, (const void *)key, keyLength);
	}
	// Start inner hash
	SHA256Init();
	for (uint8_t i = 0; i < BLOCK_LENGTH; i++) {
		SHA256Add(SHA256keyBuffer[i] ^ HMAC_IPAD);
	}
}

void SHA256HMACAdd(const uint8_t data)
{
	SHA256Add(data);
}

void SHA256HMACAdd(const uint8_t *data, size_t dataLength)
{
	SHA256Add(data, dataLength);
}

void SHA256HMACResult(uint8_t *dest)
{
	uint8_t innerHash[HASH_LENGTH];
	// Complete inner hash
	SHA256Result(innerHash);
	// Calculate outer hash
	SHA256Init();
	for (uint8_t i = 0; i < BLOCK_LENGTH; i++) {
		SHA256Add(SHA256keyBuffer[i] ^ HMAC_OPAD);
	}
	SHA256Add(innerHash, HASH_LENGTH);
	SHA256Result(dest);
}

void SHA256HMAC(uint8_t *dest, const uint8_t *key, size_t keyLength, const uint8_t *data,
                size_t dataLength)
{
	SHA256HMACInit(key, keyLength);
	SHA256HMACAdd(data, dataLength);
	SHA256HMACResult(dest);
}
