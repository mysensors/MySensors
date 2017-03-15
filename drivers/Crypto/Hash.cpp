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

#include "Hash.h"
#include <string.h>

/**
 * \class Hash Hash.h <Hash.h>
 * \brief Abstract base class for cryptographic hash algorithms.
 *
 * \sa SHA256, SHA3_256, BLAKE2s
 */

/**
 * \brief Constructs a new hash object.
 */
Hash::Hash()
{
}

/**
 * \brief Destroys this hash object.
 *
 * \note Subclasses are responsible for clearing any sensitive data
 * that remains in the hash object when it is destroyed.
 *
 * \sa clear()
 */
Hash::~Hash()
{
}

/**
 * \fn size_t Hash::hashSize() const
 * \brief Size of the hash result from finalize().
 *
 * \sa finalize(), blockSize()
 */

/**
 * \fn size_t Hash::blockSize() const
 * \brief Size of the internal block used by the hash algorithm.
 *
 * \sa update(), hashSize()
 */

/**
 * \fn void Hash::reset()
 * \brief Resets the hash ready for a new hashing process.
 *
 * \sa update(), finalize(), resetHMAC()
 */

/**
 * \fn void Hash::update(const void *data, size_t len)
 * \brief Updates the hash with more data.
 *
 * \param data Data to be hashed.
 * \param len Number of bytes of data to be hashed.
 *
 * If finalize() has already been called, then the behavior of update() will
 * be undefined.  Call reset() first to start a new hashing process.
 *
 * \sa reset(), finalize()
 */

/**
 * \fn void Hash::finalize(void *hash, size_t len)
 * \brief Finalizes the hashing process and returns the hash.
 *
 * \param hash The buffer to return the hash value in.
 * \param len The length of the \a hash buffer, normally hashSize().
 *
 * If \a len is less than hashSize(), then the hash value will be
 * truncated to the first \a len bytes.  If \a len is greater than
 * hashSize(), then the remaining bytes will left unchanged.
 *
 * If finalize() is called again, then the returned \a hash value is
 * undefined.  Call reset() first to start a new hashing process.
 *
 * \sa reset(), update(), finalizeHMAC()
 */

/**
 * \fn void Hash::clear()
 * \brief Clears the hash state, removing all sensitive data, and then
 * resets the hash ready for a new hashing process.
 *
 * \sa reset()
 */

/**
 * \fn void Hash::resetHMAC(const void *key, size_t keyLen)
 * \brief Resets the hash ready for a new HMAC hashing process.
 *
 * \param key Points to the HMAC key for the hashing process.
 * \param keyLen Size of the HMAC \a key in bytes.
 *
 * The following example computes a HMAC over a series of data blocks
 * with a specific key:
 *
 * \code
 * hash.resetHMAC(key, sizeof(key));
 * hash.update(data1, sizeof(data1));
 * hash.update(data2, sizeof(data2));
 * ...
 * hash.update(dataN, sizeof(dataN));
 * hash.finalizeHMAC(key, sizeof(key), hmac, sizeof(hmac));
 * \endcode
 *
 * The same key must be passed to both resetHMAC() and finalizeHMAC().
 *
 * \sa finalizeHMAC(), reset()
 */

/**
 * \fn void Hash::finalizeHMAC(const void *key, size_t keyLen, void *hash, size_t hashLen)
 * \brief Finalizes the HMAC hashing process and returns the hash.
 *
 * \param key Points to the HMAC key for the hashing process.  The contents
 * of this array must be identical to the value passed to resetHMAC().
 * \param keyLen Size of the HMAC \a key in bytes.
 * \param hash The buffer to return the hash value in.
 * \param hashLen The length of the \a hash buffer, normally hashSize().
 *
 * \sa resetHMAC(), finalize()
 */

/**
 * \brief Formats a HMAC key into a block.
 *
 * \param block The block to format the key into.  Must be at least
 * blockSize() bytes in length.
 * \param key Points to the HMAC key for the hashing process.
 * \param len Length of the HMAC \a key in bytes.
 * \param pad Inner (0x36) or outer (0x5C) padding value to XOR with
 * the formatted HMAC key.
 *
 * This function is intended to help subclasses implement resetHMAC() and
 * finalizeHMAC() by directly formatting the HMAC key into the subclass's
 * internal block buffer and resetting the hash.
 */
void Hash::formatHMACKey(void *block, const void *key, size_t len, uint8_t pad)
{
	size_t size = blockSize();
	reset();
	if (len <= size) {
		memcpy(block, key, len);
	} else {
		update(key, len);
		len = hashSize();
		finalize(block, len);
		reset();
	}
	uint8_t *b = (uint8_t *)block;
	memset(b + len, pad, size - len);
	while (len > 0) {
		*b++ ^= pad;
		--len;
	}
}
