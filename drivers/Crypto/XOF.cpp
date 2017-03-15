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

#include "XOF.h"

/**
 * \class XOF XOF.h <XOF.h>
 * \brief Abstract base class for Extendable-Output Functions (XOFs).
 *
 * Extendable-Output Functions, or XOFs, are a new class of cryptographic
 * primitive that was defined by NIST during the SHA-3 standardization
 * process.  Essentially an XOF is a hash algorithm that has an
 * arbitrary-length output instead of a fixed-length digest.
 *
 * XOFs can be used for a variety of cryptographic tasks:
 *
 * \li Mask generation functions for RSA OAEP style padding.
 * \li Key derivation functions for expanding key seed material into
 * arbitrary amounts of keying material for a secure session.
 * \li Stream ciphers based on a key and IV.
 *
 * To use an XOF, it is first reset() and then data is added via multiple
 * calls to update():
 *
 * \code
 * SHAKE256 xof;
 * xof.reset();
 * xof.update(data1, sizeof(data1));
 * xof.update(data2, sizeof(data2));
 * ...
 * \endcode
 *
 * Once all input data has been added, the XOF switches into extend mode
 * to generate the arbitrary-length output data:
 *
 * \code
 * xof.extend(output1, sizeof(output1));
 * xof.extend(output2, sizeof(output2));
 * ...
 * \endcode
 *
 * Mask generation and key derivation is achieved as follows, where the
 * key is unique for each invocation:
 *
 * \code
 * SHAKE256 xof;
 * xof.reset();
 * xof.update(key, sizeof(key));
 * xof.extend(output, sizeof(output));
 * \endcode
 *
 * Stream ciphers can be constructed as follows, using the special
 * encrypt() function that XOR's the output of extend() with the
 * input plaintext to generate the output ciphertext (or alternatively
 * XOR's the output of extend() with the ciphertext to recover the
 * plaintext):
 *
 * \code
 * SHAKE256 xof;
 * xof.reset();
 * xof.update(key, sizeof(key));
 * xof.update(iv, sizeof(iv));
 * xof.encrypt(output1, input1, sizeof(input1));
 * xof.encrypt(output2, input2, sizeof(input2));
 * ...
 * \endcode
 *
 * If the key is reused, then the IV must be different for each session
 * or the encryption scheme can be easily broken.  It is better to
 * generate a new key and IV combination for every session.
 *
 * It may also be a good idea to include some tag information with the input
 * data to distinguish different uses of the XOF.  For example:
 *
 * \code
 * SHAKE256 xof;
 * xof.reset();
 * xof.update(key, sizeof(key));
 * xof.update(iv, sizeof(iv));
 * xof.update("MyCrypt", 7);
 * xof.encrypt(output, input, sizeof(input));
 * \endcode
 *
 * If the same key and IV was used with a different package, then it would
 * not generate the same output as "MyCrypt".
 *
 * NIST warns that XOFs should not be used in place of hash functions.
 * This is because of related outputs: if the same input is provided to
 * an XOF with different output lengths, then the shorter output will
 * be a prefix of the larger.  This breaks the expected collision-resistance
 * of regular hash functions.  There is typically no need to use an XOF
 * for hashing because NIST has already defined SHA3_256 and SHA3_512
 * for that purpose.
 *
 * Reference: http://en.wikipedia.org/wiki/SHA-3
 *
 * \sa SHAKE256, SHAKE128, SHA3_256
 */

/**
 * \brief Constructs a new XOF object.
 */
XOF::XOF()
{
}

/**
 * \brief Destroys this XOF object.
 *
 * \note Subclasses are responsible for clearing any sensitive data
 * that remains in the XOF object when it is destroyed.
 *
 * \sa clear()
 */
XOF::~XOF()
{
}

/**
 * \fn size_t XOF::blockSize() const
 * \brief Size of the internal block used by the XOF algorithm, in bytes.
 *
 * \sa update()
 */

/**
 * \fn void XOF::reset()
 * \brief Resets the XOF ready for a new session.
 *
 * \sa update(), extend(), encrypt()
 */

/**
 * \fn void XOF::update(const void *data, size_t len)
 * \brief Updates the XOF with more data.
 *
 * \param data Data to be hashed.
 * \param len Number of bytes of data to be added to the XOF.
 *
 * If extend() or encrypt() has already been called, then the behavior of
 * update() will be undefined.  Call reset() first to start a new session.
 *
 * \sa reset(), extend(), encrypt()
 */

/**
 * \fn void XOF::extend(uint8_t *data, size_t len)
 * \brief Generates extendable output from this XOF.
 *
 * \param data The data buffer to be filled.
 * \param len The number of bytes to write to \a data.
 *
 * \sa reset(), update(), encrypt()
 */

/**
 * \fn void XOF::encrypt(uint8_t *output, const uint8_t *input, size_t len)
 * \brief Encrypts an input buffer with extendable output from this XOF.
 *
 * \param output The output buffer to write to, which may be the same
 * buffer as \a input.  The \a output buffer must have at least as many
 * bytes as the \a input buffer.
 * \param input The input buffer to read from.
 * \param len The number of bytes to encrypt.
 *
 * This function is a convenience that generates data with extend() and
 * then XOR's it with the contents of \a input to generate the \a output.
 * This function can also be used to decrypt.
 *
 * The encrypt() function can be called multiple times with different
 * regions of the plaintext data.
 *
 * \sa reset(), update(), extend(), decrypt()
 */

/**
 * \fn void XOF::decrypt(uint8_t *output, const uint8_t *input, size_t len)
 * \brief Decrypts an input buffer with extendable output from this XOF.
 *
 * \param output The output buffer to write to, which may be the same
 * buffer as \a input.  The \a output buffer must have at least as many
 * bytes as the \a input buffer.
 * \param input The input buffer to read from.
 * \param len The number of bytes to encrypt.
 *
 * This is a convenience function that merely calls encrypt().
 *
 * \sa reset(), update(), extend(), encrypt()
 */

/**
 * \fn void XOF::clear()
 * \brief Clears the hash state, removing all sensitive data, and then
 * resets the XOF ready for a new session.
 *
 * \sa reset()
 */
