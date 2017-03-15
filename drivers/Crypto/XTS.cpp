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

#include "XTS.h"
#include "Crypto.h"
#include "GF128.h"
#include <string.h>

/**
 * \class XTSCommon XTS.h <XTS.h>
 * \brief Concrete base class to assist with implementing XTS mode for
 * 128-bit block ciphers.
 *
 * References: <a href="http://libeccio.di.unisa.it/Crypto14/Lab/p1619.pdf">IEEE Std. 1619-2007</a>, <a href="http://csrc.nist.gov/publications/nistpubs/800-38E/nist-sp-800-38E.pdf">NIST SP 800-38E</a>, a href="http://web.cs.ucdavis.edu/~rogaway/papers/offsets.pdf">XEX</a>.
 *
 * \sa XTS, XTSSingleKey
 */

/**
 * \brief Constructs an XTS object with a default sector size of 512 bytes.
 */
XTSCommon::XTSCommon()
	: sectSize(512)
{
}

/**
 * \brief Clears all sensitive information and destroys this object.
 */
XTSCommon::~XTSCommon()
{
	clean(twk);
}

/**
 * \brief Gets the size of the key for XTS mode.
 *
 * The key size for XTS mode is twice the size of the underlying
 * block cipher key size.
 *
 * \sa setKey(), tweakSize()
 */
size_t XTSCommon::keySize() const
{
	return blockCipher1->keySize() * 2;
}

/**
 * \brief Gets the maximum supported size for the tweak.
 *
 * This function returns 16, which indicates that any tweak up to 16 bytes
 * in size can be specified via setTweak().
 */
size_t XTSCommon::tweakSize() const
{
	return 16;
}

/**
 * \fn size_t XTSCommon::sectorSize() const
 * \brief Gets the size of sectors encrypted or decrypted by this class.
 *
 * The default value is 512 bytes.
 *
 * \sa setSectorSize()
 */

/**
 * \brief Sets the size of sectors encrypted or decrypted by this class.
 *
 * \param size The sector size in bytes, which must be greater than or
 * equal to 16.
 *
 * \return Returns false if \a size is less than 16.
 *
 * \sa sectorSize(), encryptSector()
 */
bool XTSCommon::setSectorSize(size_t size)
{
	if (size < 16) {
		return false;
	}
	sectSize = size;
	return true;
}

/**
 * \brief Sets the key to use for XTS mode.
 *
 * \param key Points to the key.
 * \param len The size of the key in bytes which must be twice the
 * size of the underlying block cipher's key size.
 *
 * \return Returns true if the key was set or false if \a len was incorrect.
 *
 * This function should be followed by a call to setTweak() to specify
 * the sector-specific tweak.
 *
 * \sa keySize(), setTweak(), encryptSector()
 */
bool XTSCommon::setKey(const uint8_t *key, size_t len)
{
	if (!blockCipher1->setKey(key, len / 2)) {
		return false;
	}
	return blockCipher2->setKey(key + len / 2, len - (len / 2));
}

/**
 * \brief Sets the tweak value for the current sector to encrypt or decrypt.
 *
 * \param tweak Points to the tweak.
 * \param len The length of the tweak which must be less than or equal to 16.
 *
 * \return Returns true if the tweak was set or false if \a len was incorrect.
 *
 * If \a len is less than 16, then the \a tweak will be zero-padded to
 * 16 bytes.
 *
 * The \a tweak is encrypted with the second half of the XTS key to generate
 * the actual tweak value for the sector.
 *
 * \sa tweakSize(), setKey(), encryptSector()
 */
bool XTSCommon::setTweak(const uint8_t *tweak, size_t len)
{
	if (len > 16) {
		return false;
	}
	memcpy(twk, tweak, len);
	memset(((uint8_t *)twk) + len, 0, 16 - len);
	blockCipher2->encryptBlock((uint8_t *)twk, (uint8_t *)twk);
	return true;
}

#define xorTweak(output, input, tweak) \
	do { \
		for (uint8_t i = 0; i < 16; ++i) \
			(output)[i] = (input)[i] ^ ((const uint8_t *)(tweak))[i]; \
	} while (0)

/**
 * \brief Encrypts an entire sector of data.
 *
 * \param output The output buffer to write the ciphertext to, which can
 * be the same as \a input.
 * \param input The input buffer to read the plaintext from.
 *
 * The \a input and \a output buffers must be at least sectorSize()
 * bytes in length.
 *
 * \sa decryptSector(), setKey(), setTweak()
 */
void XTSCommon::encryptSector(uint8_t *output, const uint8_t *input)
{
	size_t sectLast = sectSize & ~15;
	size_t posn = 0;
	uint32_t t[4];
	memcpy(t, twk, sizeof(t));
	while (posn < sectLast) {
		// Process all complete 16-byte blocks.
		xorTweak(output, input, t);
		blockCipher1->encryptBlock(output, output);
		xorTweak(output, output, t);
		GF128::dblXTS(t);
		input += 16;
		output += 16;
		posn += 16;
	}
	if (posn < sectSize) {
		// Perform ciphertext stealing on the final partial block.
		uint8_t leftOver = sectSize - posn;
		output -= 16;
		while (leftOver > 0) {
			// Swap the left-over bytes in the last two blocks.
			--leftOver;
			uint8_t temp = input[leftOver];
			output[leftOver + 16] = output[leftOver];
			output[leftOver] = temp;
		}
		xorTweak(output, output, t);
		blockCipher1->encryptBlock(output, output);
		xorTweak(output, output, t);
	}
}

/**
 * \brief Decrypts an entire sector of data.
 *
 * \param output The output buffer to write the plaintext to, which can
 * be the same as \a input.
 * \param input The input buffer to read the ciphertext from.
 *
 * The \a input and \a output buffers must be at least sectorSize()
 * bytes in length.
 *
 * \sa encryptSector(), setKey(), setTweak()
 */
void XTSCommon::decryptSector(uint8_t *output, const uint8_t *input)
{
	size_t sectLast = sectSize & ~15;
	size_t posn = 0;
	uint32_t t[4];
	memcpy(t, twk, sizeof(t));
	if (sectLast != sectSize) {
		sectLast -= 16;
	}
	while (posn < sectLast) {
		// Process all complete 16-byte blocks.
		xorTweak(output, input, t);
		blockCipher1->decryptBlock(output, output);
		xorTweak(output, output, t);
		GF128::dblXTS(t);
		input += 16;
		output += 16;
		posn += 16;
	}
	if (posn < sectSize) {
		// Perform ciphertext stealing on the final two blocks.
		uint8_t leftOver = sectSize - 16 - posn;
		uint32_t u[4];

		// Decrypt the second-last block of ciphertext to recover
		// the last partial block of plaintext.  We need to use
		// dblXTS(t) as the tweak for this block.  Save the current
		// tweak in "u" for use later.
		memcpy(u, t, sizeof(t));
		GF128::dblXTS(t);
		xorTweak(output, input, t);
		blockCipher1->decryptBlock(output, output);
		xorTweak(output, output, t);

		// Swap the left-over bytes in the last two blocks.
		while (leftOver > 0) {
			--leftOver;
			uint8_t temp = input[leftOver + 16];
			output[leftOver + 16] = output[leftOver];
			output[leftOver] = temp;
		}

		// Decrypt the second-last block using the second-last tweak.
		xorTweak(output, output, u);
		blockCipher1->decryptBlock(output, output);
		xorTweak(output, output, u);
	}
}

/**
 * \brief Clears all security-sensitive state from this XTS object.
 */
void XTSCommon::clear()
{
	clean(twk);
	blockCipher1->clear();
	blockCipher2->clear();
}

/**
 * \fn void XTSCommon::setBlockCiphers(BlockCipher *cipher1, BlockCipher *cipher2)
 * \brief Sets the two block ciphers to use for XTS mode.
 *
 * \param cipher1 Points to the first block cipher object, which must be
 * capable of both encryption and decryption.
 * \param cipher2 Points to the second block cipher object, which must be
 * capable of both encryption but does not need to be capable of decryption.
 *
 * Both block ciphers must have a 128-bit block size.
 */

/**
 * \class XTSSingleKeyCommon XTS.h <XTS.h>
 * \brief Concrete base class to assist with implementing single-key XTS
 * mode for 128-bit block ciphers.
 *
 * References: <a href="http://libeccio.di.unisa.it/Crypto14/Lab/p1619.pdf">IEEE Std. 1619-2007</a>, <a href="http://csrc.nist.gov/publications/nistpubs/800-38E/nist-sp-800-38E.pdf">NIST SP 800-38E</a>, a href="http://web.cs.ucdavis.edu/~rogaway/papers/offsets.pdf">XEX</a>.
 *
 * \sa XTSSingleKey, XTSCommon
 */

/**
 * \fn XTSSingleKeyCommon::XTSSingleKeyCommon()
 * \brief Constructs an XTS object with a default sector size of 512 bytes.
 */

/**
 * \brief Clears all sensitive information and destroys this object.
 */
XTSSingleKeyCommon::~XTSSingleKeyCommon()
{
}

/**
 * \brief Gets the size of the key for single-pkey XTS mode.
 *
 * The key size for single-key XTS mode is the same as the key size
 * for the underlying block cipher.
 *
 * \sa setKey(), tweakSize()
 */
size_t XTSSingleKeyCommon::keySize() const
{
	return blockCipher1->keySize();
}

/**
 * \brief Sets the key to use for single-keyh XTS mode.
 *
 * \param key Points to the key.
 * \param len The size of the key in bytes which must be same as the
 * size of the underlying block cipher.
 *
 * \return Returns true if the key was set or false if \a len was incorrect.
 *
 * This function should be followed by a call to setTweak() to specify
 * the sector-specific tweak.
 *
 * \sa keySize(), setTweak(), encryptSector()
 */
bool XTSSingleKeyCommon::setKey(const uint8_t *key, size_t len)
{
	return blockCipher1->setKey(key, len);
}

/**
 * \class XTS XTS.h <XTS.h>
 * \brief Implementation of the XTS mode for 128-bit block ciphers.
 *
 * XTS mode implements the XEX tweakable block cipher mode with ciphertext
 * stealing for data that isn't a multiple of the 128-bit block size.
 *
 * XTS was designed for use in disk encryption where a large number of
 * equal-sized "sectors" need to be encrypted in a way that information
 * from one sector cannot be used to decrypt the other sectors.  The mode
 * combines the key with a sector-specific "tweak" which is usually
 * based on the sector number.
 *
 * Some Arduino systems have SD cards, but typically embedded systems
 * do not have disk drives.  However, XTS can still be useful on
 * Arduino systems with lots of EEPROM or flash memory.  If the application
 * needs to store critical security parameters like private keys then
 * XTS can be used to encrypt non-volatile memory to protect the parameters.
 *
 * The following example encrypts a sector using XTS mode:
 *
 * \code
 * XTS<AES256> xts;
 * xts.setSectorSize(520);
 * xts.setKey(key, 64); // Twice the AES256 key size.
 * xts.setTweak(sectorNumber, sizeof(sectorNumber));
 * xts.encryptSector(output, input);
 * \endcode
 *
 * XTS keys are twice the size of the underlying block cipher
 * (AES256 in the above example).  The XTS key is divided into two halves.
 * The first half is used to encrypt the plaintext and the second half
 * is used to encrypt the sector-specific tweak.  The same key can be
 * used for both, in which case XTS is equivalent to the original
 * XEX design upon which XTS was based.  The companion XTSSingleKey class
 * can be used for single-key scenarios.
 *
 * The template parameter must be a concrete subclass of BlockCipher
 * indicating the specific block cipher to use.  The example above uses
 * AES256 as the underlying cipher.
 *
 * It is also possible to specify two different block ciphers, as long as
 * they have the same key size.  Because the second half of the key is only
 * used to encrypt tweaks and never decrypt, a reduced block cipher
 * implementation like SpeckTiny that only supports encryption can be
 * used for the second block cipher:
 *
 * \code
 * XTS<SpeckSmall, SpeckTiny> xts;
 * \endcode
 *
 * This might save some memory that would otherwise be needed for the
 * decryption key schedule of the second block cipher.  XTSSingleKey provides
 * another method to save memory.
 *
 * References: <a href="http://libeccio.di.unisa.it/Crypto14/Lab/p1619.pdf">IEEE Std. 1619-2007</a>, <a href="http://csrc.nist.gov/publications/nistpubs/800-38E/nist-sp-800-38E.pdf">NIST SP 800-38E</a>, a href="http://web.cs.ucdavis.edu/~rogaway/papers/offsets.pdf">XEX</a>.
 *
 * \sa XTSSingleKey, XTSCommon
 */

/**
 * \fn XTS::XTS()
 * \brief Constructs an object for encrypting sectors in XTS mode.
 *
 * This constructor should be followed by a call to setSectorSize().
 * The default sector size is 512 bytes.
 */

/**
 * \fn XTS::~XTS()
 * \brief Clears all sensitive information and destroys this object.
 */

/**
 * \class XTSSingleKey XTS.h <XTS.h>
 * \brief Implementation of the single-key XTS mode for 128-bit block ciphers.
 *
 * XTS mode normally uses two keys to encrypt plaintext and the
 * sector-specific tweak values.  This class uses the same key for
 * both purposes, which can help save memory.
 *
 * References: <a href="http://libeccio.di.unisa.it/Crypto14/Lab/p1619.pdf">IEEE Std. 1619-2007</a>, <a href="http://csrc.nist.gov/publications/nistpubs/800-38E/nist-sp-800-38E.pdf">NIST SP 800-38E</a>, a href="http://web.cs.ucdavis.edu/~rogaway/papers/offsets.pdf">XEX</a>.
 *
 * \sa XTS, XTSSingleKeyCommon
 */

/**
 * \fn XTSSingleKey::XTSSingleKey()
 * \brief Constructs an object for encrypting sectors in XTS mode
 * with a single key instead of two split keys.
 *
 * This constructor should be followed by a call to setSectorSize().
 * The default sector size is 512 bytes.
 */

/**
 * \fn XTSSingleKey::~XTSSingleKey()
 * \brief Clears all sensitive information and destroys this object.
 */
