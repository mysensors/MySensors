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
 * \class AES192 AES.h <AES.h>
 * \brief AES block cipher with 192-bit keys.
 *
 * \sa AES128, AES256
 */

/**
 * \brief Constructs an AES 192-bit block cipher with no initial key.
 *
 * This constructor must be followed by a call to setKey() before the
 * block cipher can be used for encryption or decryption.
 */
AES192::AES192()
{
	rounds = 12;
	schedule = sched;
}

AES192::~AES192()
{
	clean(sched);
}

/**
 * \brief Size of a 192-bit AES key in bytes.
 * \return Always returns 24.
 */
size_t AES192::keySize() const
{
	return 24;
}

bool AES192::setKey(const uint8_t *key, size_t len)
{
	if (len != 24) {
		return false;
	}

	// Copy the key itself into the first 24 bytes of the schedule.
	uint8_t *schedule = sched;
	memcpy(schedule, key, 24);

	// Expand the key schedule until we have 208 bytes of expanded key.
	uint8_t iteration = 1;
	uint8_t n = 24;
	uint8_t w = 6;
	while (n < 208) {
		if (w == 6) {
			// Every 24 bytes (6 words) we need to apply the key schedule core.
			keyScheduleCore(schedule + 24, schedule + 20, iteration);
			schedule[24] ^= schedule[0];
			schedule[25] ^= schedule[1];
			schedule[26] ^= schedule[2];
			schedule[27] ^= schedule[3];
			++iteration;
			w = 0;
		} else {
			// Otherwise just XOR the word with the one 24 bytes previous.
			schedule[24] = schedule[20] ^ schedule[0];
			schedule[25] = schedule[21] ^ schedule[1];
			schedule[26] = schedule[22] ^ schedule[2];
			schedule[27] = schedule[23] ^ schedule[3];
		}

		// Advance to the next word in the schedule.
		schedule += 4;
		n += 4;
		++w;
	}

	return true;
}
