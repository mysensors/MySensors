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

#include "AES.h"
#include "Crypto.h"
#include <string.h>

/**
 * \class AES256 AES.h <AES.h>
 * \brief AES block cipher with 256-bit keys.
 *
 * \sa AES128, AES192
 */

/**
 * \brief Constructs an AES 256-bit block cipher with no initial key.
 *
 * This constructor must be followed by a call to setKey() before the
 * block cipher can be used for encryption or decryption.
 */
AES256::AES256()
{
	rounds = 14;
	schedule = sched;
}

AES256::~AES256()
{
	clean(sched);
}

/**
 * \brief Size of a 256-bit AES key in bytes.
 * \return Always returns 32.
 */
size_t AES256::keySize() const
{
	return 32;
}

bool AES256::setKey(const uint8_t *key, size_t len)
{
	if (len != 32) {
		return false;
	}

	// Copy the key itself into the first 32 bytes of the schedule.
	uint8_t *schedule = sched;
	memcpy(schedule, key, 32);

	// Expand the key schedule until we have 240 bytes of expanded key.
	uint8_t iteration = 1;
	uint8_t n = 32;
	uint8_t w = 8;
	while (n < 240) {
		if (w == 8) {
			// Every 32 bytes (8 words) we need to apply the key schedule core.
			keyScheduleCore(schedule + 32, schedule + 28, iteration);
			schedule[32] ^= schedule[0];
			schedule[33] ^= schedule[1];
			schedule[34] ^= schedule[2];
			schedule[35] ^= schedule[3];
			++iteration;
			w = 0;
		} else if (w == 4) {
			// At the 16 byte mark we need to apply the S-box.
			applySbox(schedule + 32, schedule + 28);
			schedule[32] ^= schedule[0];
			schedule[33] ^= schedule[1];
			schedule[34] ^= schedule[2];
			schedule[35] ^= schedule[3];
		} else {
			// Otherwise just XOR the word with the one 32 bytes previous.
			schedule[32] = schedule[28] ^ schedule[0];
			schedule[33] = schedule[29] ^ schedule[1];
			schedule[34] = schedule[30] ^ schedule[2];
			schedule[35] = schedule[31] ^ schedule[3];
		}

		// Advance to the next word in the schedule.
		schedule += 4;
		n += 4;
		++w;
	}

	return true;
}
