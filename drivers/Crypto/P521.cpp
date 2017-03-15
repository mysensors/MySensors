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

#include "P521.h"
#include "Crypto.h"
#include "RNG.h"
#include "SHA512.h"
#include "utility/LimbUtil.h"
#include <string.h>

/**
 * \class P521 P521.h <P521.h>
 * \brief Elliptic curve operations with the NIST P-521 curve.
 *
 * This class supports both ECDH key exchange and ECDSA signatures.
 *
 * \note The public functions in this class need a substantial amount of
 * stack space to store intermediate results while the curve function is
 * being evaluated.  About 2k of free stack space is recommended for safety.
 *
 * References: NIST FIPS 186-4,
 * <a href="http://tools.ietf.org/html/rfc6090">RFC 6090</a>,
 * <a href="http://tools.ietf.org/html/rfc6979">RFC 6979</a>,
 * <a href="http://tools.ietf.org/html/rfc6090">RFC 5903</a>
 *
 * \sa Curve25519
 */

// Number of limbs that are needed to represent a 521-bit number.
#define NUM_LIMBS_521BIT    NUM_LIMBS_BITS(521)

// Number of limbs that are needed to represent a 1042-bit number.
// To simply things we also require that this be twice the size of
// NUM_LIMB_521BIT which involves a little wastage at the high end
// of one extra limb for 8-bit and 32-bit limbs.  There is no
// wastage for 16-bit limbs.
#define NUM_LIMBS_1042BIT   (NUM_LIMBS_BITS(521) * 2)

// The overhead of clean() calls in mul(), etc can add up to a lot of
// processing time.  Only do such cleanups if strict mode has been enabled.
#if defined(P521_STRICT_CLEAN)
#define strict_clean(x)     clean(x)
#else
#define strict_clean(x)     do { ; } while (0)
#endif

// Expand the partial 9-bit left over limb at the top of a 521-bit number.
#if BIGNUMBER_LIMB_8BIT
#define LIMB_PARTIAL(value) ((uint8_t)(value)), \
	((uint8_t)((value) >> 8))
#else
#define LIMB_PARTIAL(value) (value)
#endif

/** @cond */

// The group order "q" value from RFC 4754 and RFC 5903.  This is the
// same as the "n" value from Appendix D.1.2.5 of NIST FIPS 186-4.
static limb_t const P521_q[NUM_LIMBS_521BIT] PROGMEM = {
	LIMB_PAIR(0x91386409, 0xbb6fb71e), LIMB_PAIR(0x899c47ae, 0x3bb5c9b8),
	LIMB_PAIR(0xf709a5d0, 0x7fcc0148), LIMB_PAIR(0xbf2f966b, 0x51868783),
	LIMB_PAIR(0xfffffffa, 0xffffffff), LIMB_PAIR(0xffffffff, 0xffffffff),
	LIMB_PAIR(0xffffffff, 0xffffffff), LIMB_PAIR(0xffffffff, 0xffffffff),
	LIMB_PARTIAL(0x1ff)
};

// The "b" value from Appendix D.1.2.5 of NIST FIPS 186-4.
static limb_t const P521_b[NUM_LIMBS_521BIT] PROGMEM = {
	LIMB_PAIR(0x6b503f00, 0xef451fd4), LIMB_PAIR(0x3d2c34f1, 0x3573df88),
	LIMB_PAIR(0x3bb1bf07, 0x1652c0bd), LIMB_PAIR(0xec7e937b, 0x56193951),
	LIMB_PAIR(0x8ef109e1, 0xb8b48991), LIMB_PAIR(0x99b315f3, 0xa2da725b),
	LIMB_PAIR(0xb68540ee, 0x929a21a0), LIMB_PAIR(0x8e1c9a1f, 0x953eb961),
	LIMB_PARTIAL(0x051)
};

// The "Gx" value from Appendix D.1.2.5 of NIST FIPS 186-4.
static limb_t const P521_Gx[NUM_LIMBS_521BIT] PROGMEM = {
	LIMB_PAIR(0xc2e5bd66, 0xf97e7e31), LIMB_PAIR(0x856a429b, 0x3348b3c1),
	LIMB_PAIR(0xa2ffa8de, 0xfe1dc127), LIMB_PAIR(0xefe75928, 0xa14b5e77),
	LIMB_PAIR(0x6b4d3dba, 0xf828af60), LIMB_PAIR(0x053fb521, 0x9c648139),
	LIMB_PAIR(0x2395b442, 0x9e3ecb66), LIMB_PAIR(0x0404e9cd, 0x858e06b7),
	LIMB_PARTIAL(0x0c6)
};

// The "Gy" value from Appendix D.1.2.5 of NIST FIPS 186-4.
static limb_t const P521_Gy[NUM_LIMBS_521BIT] PROGMEM = {
	LIMB_PAIR(0x9fd16650, 0x88be9476), LIMB_PAIR(0xa272c240, 0x353c7086),
	LIMB_PAIR(0x3fad0761, 0xc550b901), LIMB_PAIR(0x5ef42640, 0x97ee7299),
	LIMB_PAIR(0x273e662c, 0x17afbd17), LIMB_PAIR(0x579b4468, 0x98f54449),
	LIMB_PAIR(0x2c7d1bd9, 0x5c8a5fb4), LIMB_PAIR(0x9a3bc004, 0x39296a78),
	LIMB_PARTIAL(0x118)
};

/** @endcond */

/**
 * \brief Evaluates the curve function.
 *
 * \param result The result of applying the curve function, which consists
 * of the x and y values of the result point encoded in big-endian order.
 * \param f The scalar value to multiply by \a point to create the \a result.
 * This is assumed to be be a 521-bit number in big-endian order.
 * \param point The curve point to multiply consisting of the x and y
 * values encoded in big-endian order.  If \a point is NULL, then the
 * generator Gx and Gy values for the curve will be used instead.
 *
 * \return Returns true if \a f * \a point could be evaluated, or false if
 * \a point is not a point on the curve.
 *
 * This function provides access to the raw curve operation for testing
 * purposes.  Normally an application would use a higher-level function
 * like dh1(), dh2(), sign(), or verify().
 *
 * \sa dh1(), sign()
 */
bool P521::eval(uint8_t result[132], const uint8_t f[66], const uint8_t point[132])
{
	limb_t x[NUM_LIMBS_521BIT];
	limb_t y[NUM_LIMBS_521BIT];
	bool ok;

	// Unpack the curve point from the parameters and validate it.
	if (point) {
		BigNumberUtil::unpackBE(x, NUM_LIMBS_521BIT, point, 66);
		BigNumberUtil::unpackBE(y, NUM_LIMBS_521BIT, point + 66, 66);
		ok = validate(x, y);
	} else {
		memcpy_P(x, P521_Gx, sizeof(x));
		memcpy_P(y, P521_Gy, sizeof(y));
		ok = true;
	}

	// Evaluate the curve function.
	evaluate(x, y, f);

	// Pack the answer into the result array.
	BigNumberUtil::packBE(result, 66, x, NUM_LIMBS_521BIT);
	BigNumberUtil::packBE(result + 66, 66, y, NUM_LIMBS_521BIT);

	// Clean up.
	clean(x);
	clean(y);
	return ok;
}

/**
 * \brief Performs phase 1 of an ECDH key exchange using P-521.
 *
 * \param k The key value to send to the other party as part of the exchange.
 * \param f The generated secret value for this party.  This must not be
 * transmitted to any party or stored in permanent storage.  It only needs
 * to be kept in memory until dh2() is called.
 *
 * The \a f value is generated with \link RNGClass::rand() RNG.rand()\endlink.
 * It is the caller's responsibility to ensure that the global random number
 * pool has sufficient entropy to generate the 66 bytes of \a f safely
 * before calling this function.
 *
 * The following example demonstrates how to perform a full ECDH
 * key exchange using dh1() and dh2():
 *
 * \code
 * uint8_t f[66];
 * uint8_t k[132];
 *
 * // Generate the secret value "f" and the public value "k".
 * P521::dh1(k, f);
 *
 * // Send "k" to the other party.
 * ...
 *
 * // Read the "k" value that the other party sent to us.
 * ...
 *
 * // Generate the shared secret in "f".
 * if (!P521::dh2(k, f)) {
 *     // The received "k" value was invalid - abort the session.
 *     ...
 * }
 *
 * // The "f" value can now be used to generate session keys for encryption.
 * ...
 * \endcode
 *
 * Reference: <a href="http://tools.ietf.org/html/rfc6090">RFC 6090</a>
 *
 * \sa dh2()
 */
void P521::dh1(uint8_t k[132], uint8_t f[66])
{
	generatePrivateKey(f);
	derivePublicKey(k, f);
}

/**
 * \brief Performs phase 2 of an ECDH key exchange using P-521.
 *
 * \param k The public key value that was received from the other
 * party as part of the exchange.
 * \param f On entry, this is the secret value for this party that was
 * generated by dh1().  On exit, this will be the shared secret.
 *
 * \return Returns true if the key exchange was successful, or false if
 * the \a k value is invalid.
 *
 * Reference: <a href="http://tools.ietf.org/html/rfc6090">RFC 6090</a>
 *
 * \sa dh1()
 */
bool P521::dh2(const uint8_t k[132], uint8_t f[66])
{
	// Unpack the (x, y) point from k.
	limb_t x[NUM_LIMBS_521BIT];
	limb_t y[NUM_LIMBS_521BIT];
	BigNumberUtil::unpackBE(x, NUM_LIMBS_521BIT, k, 66);
	BigNumberUtil::unpackBE(y, NUM_LIMBS_521BIT, k + 66, 66);

	// Validate the curve point.  We keep going to preserve the timing.
	bool ok = validate(x, y);

	// Evaluate the curve function.
	evaluate(x, y, f);

	// The secret key is the x component of the final value.
	BigNumberUtil::packBE(f, 66, x, NUM_LIMBS_521BIT);

	// Clean up.
	clean(x);
	clean(y);
	return ok;
}

/**
 * \brief Signs a message using a specific P-521 private key.
 *
 * \param signature The signature value.
 * \param privateKey The private key to use to sign the message.
 * \param message Points to the message to be signed.
 * \param len The length of the \a message to be signed.
 * \param hash The hash algorithm to use to hash the \a message before signing.
 * If \a hash is NULL, then the \a message is assumed to already be a hash
 * value from some previous process.
 *
 * This function generates deterministic ECDSA signatures according to
 * RFC 6979.  The \a hash function is used to generate the k value for
 * the signature.  If \a hash is NULL, then SHA512 is used.
 * The \a hash object must be capable of HMAC mode.
 *
 * The length of the hashed message must be less than or equal to 64
 * bytes in size.  Longer messages will be truncated to 64 bytes.
 *
 * References: <a href="http://tools.ietf.org/html/rfc6090">RFC 6090</a>,
 * <a href="http://tools.ietf.org/html/rfc6979">RFC 6979</a>
 *
 * \sa verify(), generatePrivateKey()
 */
void P521::sign(uint8_t signature[132], const uint8_t privateKey[66],
                const void *message, size_t len, Hash *hash)
{
	uint8_t hm[66];
	uint8_t k[66];
	limb_t x[NUM_LIMBS_521BIT];
	limb_t y[NUM_LIMBS_521BIT];
	limb_t t[NUM_LIMBS_521BIT];
	uint64_t count = 0;

	// Format the incoming message, hashing it if necessary.
	if (hash) {
		// Hash the message.
		hash->reset();
		hash->update(message, len);
		len = hash->hashSize();
		if (len > 64) {
			len = 64;
		}
		memset(hm, 0, 66 - len);
		hash->finalize(hm + 66 - len, len);
	} else {
		// The message is the hash.
		if (len > 64) {
			len = 64;
		}
		memset(hm, 0, 66 - len);
		memcpy(hm + 66 - len, message, len);
	}

	// Keep generating k values until both r and s are non-zero.
	for (;;) {
		// Generate the k value deterministically according to RFC 6979.
		if (hash) {
			generateK(k, hm, privateKey, hash, count);
		} else {
			generateK(k, hm, privateKey, count);
		}

		// Generate r = kG.x mod q.
		memcpy_P(x, P521_Gx, sizeof(x));
		memcpy_P(y, P521_Gy, sizeof(y));
		evaluate(x, y, k);
		BigNumberUtil::reduceQuick_P(x, x, P521_q, NUM_LIMBS_521BIT);
		BigNumberUtil::packBE(signature, 66, x, NUM_LIMBS_521BIT);

		// If r is zero, then we need to generate a new k value.
		// This is utterly improbable, but let's be safe anyway.
		if (BigNumberUtil::isZero(x, NUM_LIMBS_521BIT)) {
			++count;
			continue;
		}

		// Generate s = (privateKey * r + hm) / k mod q.
		BigNumberUtil::unpackBE(y, NUM_LIMBS_521BIT, privateKey, 66);
		mulQ(y, y, x);
		BigNumberUtil::unpackBE(x, NUM_LIMBS_521BIT, hm, 66);
		BigNumberUtil::add(x, x, y, NUM_LIMBS_521BIT);
		BigNumberUtil::reduceQuick_P(x, x, P521_q, NUM_LIMBS_521BIT);
		BigNumberUtil::unpackBE(y, NUM_LIMBS_521BIT, k, 66);
		recipQ(t, y);
		mulQ(x, x, t);
		BigNumberUtil::packBE(signature + 66, 66, x, NUM_LIMBS_521BIT);

		// Exit the loop if s is non-zero.
		if (!BigNumberUtil::isZero(x, NUM_LIMBS_521BIT)) {
			break;
		}

		// We need to generate a new k value according to RFC 6979.
		// This is utterly improbable, but let's be safe anyway.
		++count;
	}

	// Clean up.
	clean(hm);
	clean(k);
	clean(x);
	clean(y);
	clean(t);
}

/**
 * \brief Verifies a signature using a specific P-521 public key.
 *
 * \param signature The signature value to be verified.
 * \param publicKey The public key to use to verify the signature.
 * \param message The message whose signature is to be verified.
 * \param len The length of the \a message to be verified.
 * \param hash The hash algorithm to use to hash the \a message before
 * verification.  If \a hash is NULL, then the \a message is assumed to
 * already be a hash value from some previous process.
 *
 * The length of the hashed message must be less than or equal to 64
 * bytes in size.  Longer messages will be truncated to 64 bytes.
 *
 * \return Returns true if the \a signature is valid for \a message;
 * or false if the \a publicKey or \a signature is not valid.
 *
 * \sa sign()
 */
bool P521::verify(const uint8_t signature[132],
                  const uint8_t publicKey[132],
                  const void *message, size_t len, Hash *hash)
{
	limb_t x[NUM_LIMBS_521BIT];
	limb_t y[NUM_LIMBS_521BIT];
	limb_t r[NUM_LIMBS_521BIT];
	limb_t s[NUM_LIMBS_521BIT];
	limb_t u1[NUM_LIMBS_521BIT];
	limb_t u2[NUM_LIMBS_521BIT];
	uint8_t t[66];
	bool ok = false;

	// Because we are operating on public values, we don't need to
	// be as strict about constant time.  Bail out early if there
	// is a problem with the parameters.

	// Unpack the signature.  The values must be between 1 and q - 1.
	BigNumberUtil::unpackBE(r, NUM_LIMBS_521BIT, signature, 66);
	BigNumberUtil::unpackBE(s, NUM_LIMBS_521BIT, signature + 66, 66);
	if (BigNumberUtil::isZero(r, NUM_LIMBS_521BIT) ||
	        BigNumberUtil::isZero(s, NUM_LIMBS_521BIT) ||
	        !BigNumberUtil::sub_P(x, r, P521_q, NUM_LIMBS_521BIT) ||
	        !BigNumberUtil::sub_P(x, s, P521_q, NUM_LIMBS_521BIT)) {
		goto failed;
	}

	// Unpack the public key and check that it is a valid curve point.
	BigNumberUtil::unpackBE(x, NUM_LIMBS_521BIT, publicKey, 66);
	BigNumberUtil::unpackBE(y, NUM_LIMBS_521BIT, publicKey + 66, 66);
	if (!validate(x, y)) {
		goto failed;
	}

	// Hash the message to generate hm, which we store into u1.
	if (hash) {
		// Hash the message.
		hash->reset();
		hash->update(message, len);
		len = hash->hashSize();
		if (len > 64) {
			len = 64;
		}
		hash->finalize(u2, len);
		BigNumberUtil::unpackBE(u1, NUM_LIMBS_521BIT, (uint8_t *)u2, len);
	} else {
		// The message is the hash.
		if (len > 64) {
			len = 64;
		}
		BigNumberUtil::unpackBE(u1, NUM_LIMBS_521BIT, (uint8_t *)message, len);
	}

	// Compute u1 = hm * s^-1 mod q and u2 = r * s^-1 mod q.
	recipQ(u2, s);
	mulQ(u1, u1, u2);
	mulQ(u2, r, u2);

	// Compute the curve point R = u2 * publicKey + u1 * G.
	BigNumberUtil::packBE(t, 66, u2, NUM_LIMBS_521BIT);
	evaluate(x, y, t);
	memcpy_P(u2, P521_Gx, sizeof(x));
	memcpy_P(s, P521_Gy, sizeof(y));
	BigNumberUtil::packBE(t, 66, u1, NUM_LIMBS_521BIT);
	evaluate(u2, s, t);
	addAffine(u2, s, x, y);

	// If R.x = r mod q, then the signature is valid.
	BigNumberUtil::reduceQuick_P(u1, u2, P521_q, NUM_LIMBS_521BIT);
	ok = secure_compare(u1, r, NUM_LIMBS_521BIT * sizeof(limb_t));

	// Clean up and exit.
failed:
	clean(x);
	clean(y);
	clean(r);
	clean(s);
	clean(u1);
	clean(u2);
	clean(t);
	return ok;
}

/**
 * \brief Generates a private key for P-521 signing operations.
 *
 * \param privateKey The resulting private key.
 *
 * The private key is generated with \link RNGClass::rand() RNG.rand()\endlink.
 * It is the caller's responsibility to ensure that the global random number
 * pool has sufficient entropy to generate the 521 bits of the key safely
 * before calling this function.
 *
 * \sa derivePublicKey(), sign()
 */
void P521::generatePrivateKey(uint8_t privateKey[66])
{
	// Generate a random 521-bit value for the private key.  The value
	// must be generated uniformly at random between 1 and q - 1 where q
	// is the group order (RFC 6090).  We use the recommended algorithm
	// from Appendix B of RFC 6090: generate a random 521-bit value
	// and discard it if it is not within the range 1 to q - 1.
	limb_t x[NUM_LIMBS_521BIT];
	do {
		RNG.rand((uint8_t *)x, sizeof(x));
#if BIGNUMBER_LIMB_8BIT
		x[NUM_LIMBS_521BIT - 1] &= 0x01;
#else
		x[NUM_LIMBS_521BIT - 1] &= 0x1FF;
#endif
		BigNumberUtil::packBE(privateKey, 66, x, NUM_LIMBS_521BIT);
	} while (BigNumberUtil::isZero(x, NUM_LIMBS_521BIT) ||
	         !BigNumberUtil::sub_P(x, x, P521_q, NUM_LIMBS_521BIT));
	clean(x);
}

/**
 * \brief Derives the public key from a private key for P-521
 * signing operations.
 *
 * \param publicKey The public key.
 * \param privateKey The private key, which is assumed to have been
 * created by generatePrivateKey().
 *
 * \sa generatePrivateKey(), verify()
 */
void P521::derivePublicKey(uint8_t publicKey[132], const uint8_t privateKey[66])
{
	// Evaluate the curve function starting with the generator.
	limb_t x[NUM_LIMBS_521BIT];
	limb_t y[NUM_LIMBS_521BIT];
	memcpy_P(x, P521_Gx, sizeof(x));
	memcpy_P(y, P521_Gy, sizeof(y));
	evaluate(x, y, privateKey);

	// Pack the (x, y) point into the public key.
	BigNumberUtil::packBE(publicKey, 66, x, NUM_LIMBS_521BIT);
	BigNumberUtil::packBE(publicKey + 66, 66, y, NUM_LIMBS_521BIT);

	// Clean up.
	clean(x);
	clean(y);
}

/**
 * \brief Validates a private key value to ensure that it is
 * between 1 and q - 1.
 *
 * \param privateKey The private key value to validate.
 * \return Returns true if \a privateKey is valid, false if not.
 *
 * \sa isValidPublicKey()
 */
bool P521::isValidPrivateKey(const uint8_t privateKey[66])
{
	// The value "q" as a byte array from most to least significant.
	static uint8_t const P521_q_bytes[66] PROGMEM = {
		0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xFF, 0xFA, 0x51, 0x86, 0x87, 0x83, 0xBF, 0x2F,
		0x96, 0x6B, 0x7F, 0xCC, 0x01, 0x48, 0xF7, 0x09,
		0xA5, 0xD0, 0x3B, 0xB5, 0xC9, 0xB8, 0x89, 0x9C,
		0x47, 0xAE, 0xBB, 0x6F, 0xB7, 0x1E, 0x91, 0x38,
		0x64, 0x09
	};
	uint8_t zeroTest = 0;
	uint8_t posn = 66;
	uint16_t borrow = 0;
	while (posn > 0) {
		--posn;

		// Check for zero.
		zeroTest |= privateKey[posn];

		// Subtract P521_q_bytes from the key.  If there is no borrow,
		// then the key value was greater than or equal to q.
		borrow = ((uint16_t)(privateKey[posn])) -
		         pgm_read_byte(&(P521_q_bytes[posn])) -
		         ((borrow >> 8) & 0x01);
	}
	return zeroTest != 0 && borrow != 0;
}

/**
 * \brief Validates a public key to ensure that it is a valid curve point.
 *
 * \param publicKey The public key value to validate.
 * \return Returns true if \a publicKey is valid, false if not.
 *
 * \sa isValidPrivateKey()
 */
bool P521::isValidPublicKey(const uint8_t publicKey[132])
{
	limb_t x[NUM_LIMBS_521BIT];
	limb_t y[NUM_LIMBS_521BIT];
	BigNumberUtil::unpackBE(x, NUM_LIMBS_521BIT, publicKey, 66);
	BigNumberUtil::unpackBE(y, NUM_LIMBS_521BIT, publicKey + 66, 66);
	bool ok = validate(x, y);
	clean(x);
	clean(y);
	return ok;
}

/**
 * \fn bool P521::isValidCurvePoint(const uint8_t point[132])
 * \brief Validates a point to ensure that it is on the curve.
 *
 * \param point The point to validate.
 * \return Returns true if \a point is valid and on the curve, false if not.
 *
 * This is a convenience function that calls isValidPublicKey() as the
 * two operations are equivalent.
 */

/**
 * \brief Evaluates the curve function by multiplying (x, y) by f.
 *
 * \param x The X co-ordinate of the curve point.  Replaced with the X
 * co-ordinate of the result on exit.
 * \param y The Y co-ordinate of the curve point.  Replaced with the Y
 * co-ordinate of the result on exit.
 * \param f The 521-bit scalar to multiply (x, y) by, most significant
 * bit first.
 */
void P521::evaluate(limb_t *x, limb_t *y, const uint8_t f[66])
{
	limb_t x1[NUM_LIMBS_521BIT];
	limb_t y1[NUM_LIMBS_521BIT];
	limb_t z1[NUM_LIMBS_521BIT];
	limb_t x2[NUM_LIMBS_521BIT];
	limb_t y2[NUM_LIMBS_521BIT];
	limb_t z2[NUM_LIMBS_521BIT];

	// We want the input in Jacobian co-ordinates.  The point (x, y, z)
	// corresponds to the affine point (x / z^2, y / z^3), so if we set z
	// to 1 we end up with Jacobian co-ordinates.  Remember that z is 1
	// and continue on.

	// Set the answer to the point-at-infinity initially (z = 0).
	memset(x1, 0, sizeof(x1));
	memset(y1, 0, sizeof(y1));
	memset(z1, 0, sizeof(z1));

	// Special handling for the highest bit.  We can skip dblPoint()/addPoint()
	// and simply conditionally move (x, y, z) into (x1, y1, z1).
	uint8_t select = (f[0] & 0x01);
	cmove(select, x1, x);
	cmove(select, y1, y);
	cmove1(select, z1); // z = 1

	// Iterate over the remaining 520 bits of f from highest to lowest.
	uint8_t mask = 0x80;
	uint8_t fposn = 1;
	for (uint16_t t = 520; t > 0; --t) {
		// Double the answer.
		dblPoint(x1, y1, z1, x1, y1, z1);

		// Add (x, y, z) to (x1, y1, z1) for the next 1 bit.
		// We must always do this to preserve the overall timing.
		// The z value is always 1 so we can omit that argument.
		addPoint(x2, y2, z2, x1, y1, z1, x, y/*, z*/);

		// If the bit was 1, then move (x2, y2, z2) into (x1, y1, z1).
		select = (f[fposn] & mask);
		cmove(select, x1, x2);
		cmove(select, y1, y2);
		cmove(select, z1, z2);

		// Move onto the next bit.
		mask >>= 1;
		if (!mask) {
			++fposn;
			mask = 0x80;
		}
	}

	// Convert from Jacobian co-ordinates back into affine co-ordinates.
	// x = x1 * (z1^2)^-1, y = y1 * (z1^3)^-1.
	recip(x2, z1);
	square(y2, x2);
	mul(x, x1, y2);
	mul(y2, y2, x2);
	mul(y, y1, y2);

	// Clean up.
	clean(x1);
	clean(y1);
	clean(z1);
	clean(x2);
	clean(y2);
	clean(z2);
}

/**
 * \brief Adds two affine points.
 *
 * \param x1 The X value for the first point to add, and the result.
 * \param y1 The Y value for the first point to add, and the result.
 * \param x2 The X value for the second point to add.
 * \param y2 The Y value for the second point to add.
 *
 * The Z values for the two points are assumed to be 1.
 */
void P521::addAffine(limb_t *x1, limb_t *y1, const limb_t *x2, const limb_t *y2)
{
	limb_t xout[NUM_LIMBS_521BIT];
	limb_t yout[NUM_LIMBS_521BIT];
	limb_t zout[NUM_LIMBS_521BIT];
	limb_t z1[NUM_LIMBS_521BIT];

	// z1 = 1
	z1[0] = 1;
	memset(z1 + 1, 0, (NUM_LIMBS_521BIT - 1) * sizeof(limb_t));

	// Add the two points.
	addPoint(xout, yout, zout, x1, y1, z1, x2, y2/*, z2*/);

	// Convert from Jacobian co-ordinates back into affine co-ordinates.
	// x1 = xout * (zout^2)^-1, y1 = yout * (zout^3)^-1.
	recip(z1, zout);
	square(zout, z1);
	mul(x1, xout, zout);
	mul(zout, zout, z1);
	mul(y1, yout, zout);

	// Clean up.
	clean(xout);
	clean(yout);
	clean(zout);
	clean(z1);
}

/**
 * \brief Validates that (x, y) is actually a point on the curve.
 *
 * \param x The X co-ordinate of the point to test.
 * \param y The Y co-ordinate of the point to test.
 * \return Returns true if (x, y) is on the curve, or false if not.
 *
 * \sa inRange()
 */
bool P521::validate(const limb_t *x, const limb_t *y)
{
	bool result;

	// If x or y is greater than or equal to 2^521 - 1, then the
	// point is definitely not on the curve.  Preserve timing by
	// delaying the reporting of the result until later.
	result = inRange(x);
	result &= inRange(y);

	// We need to check that y^2 = x^3 - 3 * x + b mod 2^521 - 1.
	limb_t t1[NUM_LIMBS_521BIT];
	limb_t t2[NUM_LIMBS_521BIT];
	square(t1, x);
	mul(t1, t1, x);
	mulLiteral(t2, x, 3);
	sub(t1, t1, t2);
	memcpy_P(t2, P521_b, sizeof(t2));
	add(t1, t1, t2);
	square(t2, y);
	result &= secure_compare(t1, t2, sizeof(t1));
	clean(t1);
	clean(t2);
	return result;
}

/**
 * \brief Determines if a value is between 0 and 2^521 - 2.
 *
 * \param x The value to test.
 * \return Returns true if \a x is in range, false if not.
 *
 * \sa validate()
 */
bool P521::inRange(const limb_t *x)
{
	// Do a trial subtraction of 2^521 - 1 from x, which is equivalent
	// to adding 1 and subtracting 2^521.  We only need the carry.
	dlimb_t carry = 1;
	limb_t word = 0;
	for (uint8_t index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry += *x++;
		word = (limb_t)carry;
		carry >>= LIMB_BITS;
	}

	// Determine the carry out from the low 521 bits.
#if BIGNUMBER_LIMB_8BIT
	carry = (carry << 7) + (word >> 1);
#else
	carry = (carry << (LIMB_BITS - 9)) + (word >> 9);
#endif

	// If the carry is zero, then x was in range.  Otherwise it is out
	// of range.  Check for zero in a way that preserves constant timing.
	word = (limb_t)(carry | (carry >> LIMB_BITS));
	word = (limb_t)(((((dlimb_t)1) << LIMB_BITS) - word) >> LIMB_BITS);
	return (bool)word;
}

/**
 * \brief Reduces a number modulo 2^521 - 1.
 *
 * \param result The array that will contain the result when the
 * function exits.  Must be NUM_LIMBS_521BIT limbs in size.
 * \param x The number to be reduced, which must be NUM_LIMBS_1042BIT
 * limbs in size and less than square(2^521 - 1).  This array can be
 * the same as \a result.
 */
void P521::reduce(limb_t *result, const limb_t *x)
{
#if BIGNUMBER_LIMB_16BIT || BIGNUMBER_LIMB_32BIT || BIGNUMBER_LIMB_64BIT
	// According to NIST FIPS 186-4, we add the high 521 bits to the
	// low 521 bits and then do a trial subtraction of 2^521 - 1.
	// We do both in a single step.  Subtracting 2^521 - 1 is equivalent
	// to adding 1 and subtracting 2^521.
	uint8_t index;
	const limb_t *xl = x;
	const limb_t *xh = x + NUM_LIMBS_521BIT;
	limb_t *rr = result;
	dlimb_t carry;
	limb_t word = x[NUM_LIMBS_521BIT - 1];
	carry = (word >> 9) + 1;
	word &= 0x1FF;
	for (index = 0; index < (NUM_LIMBS_521BIT - 1); ++index) {
		carry += *xl++;
		carry += ((dlimb_t)(*xh++)) << (LIMB_BITS - 9);
		*rr++ = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	carry += word;
	carry += ((dlimb_t)(x[NUM_LIMBS_1042BIT - 1])) << (LIMB_BITS - 9);
	word = (limb_t)carry;
	*rr = word;

	// If the carry out was 1, then mask it off and we have the answer.
	// If the carry out was 0, then we need to add 2^521 - 1 back again.
	// To preserve the timing we perform a conditional subtract of 1 and
	// then mask off the high bits.
	carry = ((word >> 9) ^ 0x01) & 0x01;
	rr = result;
	for (index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry = ((dlimb_t)(*rr)) - carry;
		*rr++ = (limb_t)carry;
		carry = (carry >> LIMB_BITS) & 0x01;
	}
	*(--rr) &= 0x1FF;
#elif BIGNUMBER_LIMB_8BIT
	// Same as above, but for 8-bit limbs.
	uint8_t index;
	const limb_t *xl = x;
	const limb_t *xh = x + NUM_LIMBS_521BIT;
	limb_t *rr = result;
	dlimb_t carry;
	limb_t word = x[NUM_LIMBS_521BIT - 1];
	carry = (word >> 1) + 1;
	word &= 0x01;
	for (index = 0; index < (NUM_LIMBS_521BIT - 1); ++index) {
		carry += *xl++;
		carry += ((dlimb_t)(*xh++)) << 7;
		*rr++ = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	carry += word;
	carry += ((dlimb_t)(x[NUM_LIMBS_1042BIT - 1])) << 1;
	word = (limb_t)carry;
	*rr = word;
	carry = ((word >> 1) ^ 0x01) & 0x01;
	rr = result;
	for (index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry = ((dlimb_t)(*rr)) - carry;
		*rr++ = (limb_t)carry;
		carry = (carry >> LIMB_BITS) & 0x01;
	}
	*(--rr) &= 0x01;
#else
#error "Don't know how to reduce values mod 2^521 - 1"
#endif
}

/**
 * \brief Quickly reduces a number modulo 2^521 - 1.
 *
 * \param x The number to be reduced, which must be NUM_LIMBS_521BIT
 * limbs in size and less than or equal to 2 * (2^521 - 2).
 *
 * The answer is also put into \a x and will consist of NUM_LIMBS_521BIT limbs.
 *
 * This function is intended for reducing the result of additions where
 * the caller knows that \a x is within the described range.  A single
 * trial subtraction is all that is needed to reduce the number.
 */
void P521::reduceQuick(limb_t *x)
{
	// Perform a trial subtraction of 2^521 - 1 from x.  This is
	// equivalent to adding 1 and subtracting 2^521 - 1.
	uint8_t index;
	limb_t *xx = x;
	dlimb_t carry = 1;
	for (index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry += *xx;
		*xx++ = (limb_t)carry;
		carry >>= LIMB_BITS;
	}

	// If the carry out was 1, then mask it off and we have the answer.
	// If the carry out was 0, then we need to add 2^521 - 1 back again.
	// To preserve the timing we perform a conditional subtract of 1 and
	// then mask off the high bits.
#if BIGNUMBER_LIMB_16BIT || BIGNUMBER_LIMB_32BIT || BIGNUMBER_LIMB_64BIT
	carry = ((x[NUM_LIMBS_521BIT - 1] >> 9) ^ 0x01) & 0x01;
	xx = x;
	for (index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry = ((dlimb_t)(*xx)) - carry;
		*xx++ = (limb_t)carry;
		carry = (carry >> LIMB_BITS) & 0x01;
	}
	*(--xx) &= 0x1FF;
#elif BIGNUMBER_LIMB_8BIT
	carry = ((x[NUM_LIMBS_521BIT - 1] >> 1) ^ 0x01) & 0x01;
	xx = x;
	for (index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry = ((dlimb_t)(*xx)) - carry;
		*xx++ = (limb_t)carry;
		carry = (carry >> LIMB_BITS) & 0x01;
	}
	*(--xx) &= 0x01;
#endif
}

/**
 * \brief Multiplies two 521-bit values to produce a 1042-bit result.
 *
 * \param result The result, which must be NUM_LIMBS_1042BIT limbs in size
 * and must not overlap with \a x or \a y.
 * \param x The first value to multiply, which must be NUM_LIMBS_521BIT
 * limbs in size.
 * \param y The second value to multiply, which must be NUM_LIMBS_521BIT
 * limbs in size.
 *
 * \sa mul()
 */
void P521::mulNoReduce(limb_t *result, const limb_t *x, const limb_t *y)
{
	uint8_t i, j;
	dlimb_t carry;
	limb_t word;
	const limb_t *yy;
	limb_t *rr;

	// Multiply the lowest word of x by y.
	carry = 0;
	word = x[0];
	yy = y;
	rr = result;
	for (i = 0; i < NUM_LIMBS_521BIT; ++i) {
		carry += ((dlimb_t)(*yy++)) * word;
		*rr++ = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	*rr = (limb_t)carry;

	// Multiply and add the remaining words of x by y.
	for (i = 1; i < NUM_LIMBS_521BIT; ++i) {
		word = x[i];
		carry = 0;
		yy = y;
		rr = result + i;
		for (j = 0; j < NUM_LIMBS_521BIT; ++j) {
			carry += ((dlimb_t)(*yy++)) * word;
			carry += *rr;
			*rr++ = (limb_t)carry;
			carry >>= LIMB_BITS;
		}
		*rr = (limb_t)carry;
	}
}

/**
 * \brief Multiplies two values and then reduces the result modulo 2^521 - 1.
 *
 * \param result The result, which must be NUM_LIMBS_521BIT limbs in size
 * and can be the same array as \a x or \a y.
 * \param x The first value to multiply, which must be NUM_LIMBS_521BIT limbs
 * in size and less than 2^521 - 1.
 * \param y The second value to multiply, which must be NUM_LIMBS_521BIT limbs
 * in size and less than 2^521 - 1.  This can be the same array as \a x.
 */
void P521::mul(limb_t *result, const limb_t *x, const limb_t *y)
{
	limb_t temp[NUM_LIMBS_1042BIT];
	mulNoReduce(temp, x, y);
	reduce(result, temp);
	strict_clean(temp);
}

/**
 * \fn void P521::square(limb_t *result, const limb_t *x)
 * \brief Squares a value and then reduces it modulo 2^521 - 1.
 *
 * \param result The result, which must be NUM_LIMBS_521BIT limbs in size and
 * can be the same array as \a x.
 * \param x The value to square, which must be NUM_LIMBS_521BIT limbs in size
 * and less than 2^521 - 1.
 */

/**
 * \brief Multiply a value by a single-limb literal modulo 2^521 - 1.
 *
 * \param result The result, which must be NUM_LIMBS_521BIT limbs in size and
 * can be the same array as \a x.
 * \param x The first value to multiply, which must be NUM_LIMBS_521BIT limbs
 * in size and less than 2^521 - 1.
 * \param y The second value to multiply, which must be less than 128.
 */
void P521::mulLiteral(limb_t *result, const limb_t *x, limb_t y)
{
	uint8_t index;
	dlimb_t carry = 0;
	const limb_t *xx = x;
	limb_t *rr = result;

	// Multiply x by the literal and put it into the result array.
	// We assume that y is small enough that overflow from the
	// highest limb will not occur during this process.
	for (index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry += ((dlimb_t)(*xx++)) * y;
		*rr++ = (limb_t)carry;
		carry >>= LIMB_BITS;
	}

	// Reduce the value modulo 2^521 - 1.  The high half is only a
	// single limb, so we can short-cut some of reduce() here.
#if BIGNUMBER_LIMB_16BIT || BIGNUMBER_LIMB_32BIT || BIGNUMBER_LIMB_64BIT
	limb_t word = result[NUM_LIMBS_521BIT - 1];
	carry = (word >> 9) + 1;
	word &= 0x1FF;
	rr = result;
	for (index = 0; index < (NUM_LIMBS_521BIT - 1); ++index) {
		carry += *rr;
		*rr++ = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	carry += word;
	word = (limb_t)carry;
	*rr = word;

	// If the carry out was 1, then mask it off and we have the answer.
	// If the carry out was 0, then we need to add 2^521 - 1 back again.
	// To preserve the timing we perform a conditional subtract of 1 and
	// then mask off the high bits.
	carry = ((word >> 9) ^ 0x01) & 0x01;
	rr = result;
	for (index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry = ((dlimb_t)(*rr)) - carry;
		*rr++ = (limb_t)carry;
		carry = (carry >> LIMB_BITS) & 0x01;
	}
	*(--rr) &= 0x1FF;
#elif BIGNUMBER_LIMB_8BIT
	// Same as above, but for 8-bit limbs.
	limb_t word = result[NUM_LIMBS_521BIT - 1];
	carry = (word >> 1) + 1;
	word &= 0x01;
	rr = result;
	for (index = 0; index < (NUM_LIMBS_521BIT - 1); ++index) {
		carry += *rr;
		*rr++ = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	carry += word;
	word = (limb_t)carry;
	*rr = word;
	carry = ((word >> 1) ^ 0x01) & 0x01;
	rr = result;
	for (index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry = ((dlimb_t)(*rr)) - carry;
		*rr++ = (limb_t)carry;
		carry = (carry >> LIMB_BITS) & 0x01;
	}
	*(--rr) &= 0x01;
#endif
}

/**
 * \brief Adds two values and then reduces the result modulo 2^521 - 1.
 *
 * \param result The result, which must be NUM_LIMBS_521BIT limbs in size
 * and can be the same array as \a x or \a y.
 * \param x The first value to multiply, which must be NUM_LIMBS_521BIT
 * limbs in size and less than 2^521 - 1.
 * \param y The second value to multiply, which must be NUM_LIMBS_521BIT
 * limbs in size and less than 2^521 - 1.
 */
void P521::add(limb_t *result, const limb_t *x, const limb_t *y)
{
	dlimb_t carry = 0;
	limb_t *rr = result;
	for (uint8_t posn = 0; posn < NUM_LIMBS_521BIT; ++posn) {
		carry += *x++;
		carry += *y++;
		*rr++ = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
	reduceQuick(result);
}

/**
 * \brief Subtracts two values and then reduces the result modulo 2^521 - 1.
 *
 * \param result The result, which must be NUM_LIMBS_521BIT limbs in size
 * and can be the same array as \a x or \a y.
 * \param x The first value to multiply, which must be NUM_LIMBS_521BIT
 * limbs in size and less than 2^521 - 1.
 * \param y The second value to multiply, which must be NUM_LIMBS_521BIT
 * limbs in size and less than 2^521 - 1.
 */
void P521::sub(limb_t *result, const limb_t *x, const limb_t *y)
{
	dlimb_t borrow;
	uint8_t posn;
	limb_t *rr = result;

	// Subtract y from x to generate the intermediate result.
	borrow = 0;
	for (posn = 0; posn < NUM_LIMBS_521BIT; ++posn) {
		borrow = ((dlimb_t)(*x++)) - (*y++) - ((borrow >> LIMB_BITS) & 0x01);
		*rr++ = (limb_t)borrow;
	}

	// If we had a borrow, then the result has gone negative and we
	// have to add 2^521 - 1 to the result to make it positive again.
	// The top bits of "borrow" will be all 1's if there is a borrow
	// or it will be all 0's if there was no borrow.  Easiest is to
	// conditionally subtract 1 and then mask off the high bits.
	rr = result;
	borrow = (borrow >> LIMB_BITS) & 1U;
	borrow = ((dlimb_t)(*rr)) - borrow;
	*rr++ = (limb_t)borrow;
	for (posn = 1; posn < NUM_LIMBS_521BIT; ++posn) {
		borrow = ((dlimb_t)(*rr)) - ((borrow >> LIMB_BITS) & 0x01);
		*rr++ = (limb_t)borrow;
	}
#if BIGNUMBER_LIMB_8BIT
	*(--rr) &= 0x01;
#else
	*(--rr) &= 0x1FF;
#endif
}

/**
 * \brief Doubles a point represented in Jacobian co-ordinates.
 *
 * \param xout The X value for the result.
 * \param yout The Y value for the result.
 * \param zout The Z value for the result.
 * \param xin The X value for the point to be doubled.
 * \param yin The Y value for the point to be doubled.
 * \param zin The Z value for the point to be doubled.
 *
 * The output parameters can be the same as the input parameters
 * to double in-place.
 *
 * Reference: http://hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#doubling-dbl-2001-b
 */
void P521::dblPoint(limb_t *xout, limb_t *yout, limb_t *zout,
                    const limb_t *xin, const limb_t *yin,
                    const limb_t *zin)
{
	limb_t alpha[NUM_LIMBS_521BIT];
	limb_t beta[NUM_LIMBS_521BIT];
	limb_t gamma[NUM_LIMBS_521BIT];
	limb_t delta[NUM_LIMBS_521BIT];
	limb_t tmp[NUM_LIMBS_521BIT];

	// Double the point.  If it is the point at infinity (z = 0),
	// then zout will still be zero at the end of this process so
	// we don't need any special handling for that case.
	square(delta, zin);       // delta = z^2
	square(gamma, yin);       // gamma = y^2
	mul(beta, xin, gamma);    // beta = x * gamma
	sub(tmp, xin, delta);     // alpha = 3 * (x - delta) * (x + delta)
	mulLiteral(alpha, tmp, 3);
	add(tmp, xin, delta);
	mul(alpha, alpha, tmp);
	square(xout, alpha);      // xout = alpha^2 - 8 * beta
	mulLiteral(tmp, beta, 8);
	sub(xout, xout, tmp);
	add(zout, yin, zin);      // zout = (y + z)^2 - gamma - delta
	square(zout, zout);
	sub(zout, zout, gamma);
	sub(zout, zout, delta);
	mulLiteral(yout, beta, 4);// yout = alpha * (4 * beta - xout) - 8 * gamma^2
	sub(yout, yout, xout);
	mul(yout, alpha, yout);
	square(gamma, gamma);
	mulLiteral(gamma, gamma, 8);
	sub(yout, yout, gamma);

	// Clean up.
	strict_clean(alpha);
	strict_clean(beta);
	strict_clean(gamma);
	strict_clean(delta);
	strict_clean(tmp);
}

/**
 * \brief Adds two curve points, one represented in Jacobian co-ordinates,
 * and the other represented in affine co-ordinates.
 *
 * \param xout The X value for the result.
 * \param yout The Y value for the result.
 * \param zout The Z value for the result.
 * \param x1 The X value for the first point to add.
 * \param y1 The Y value for the first point to add.
 * \param z1 The Z value for the first point to add.
 * \param x2 The X value for the second point to add.
 * \param y2 The Y value for the second point to add.
 *
 * The output parameters must not overlap with either of the inputs.
 *
 * The Z value of the second point is implicitly assumed to be 1.
 *
 * Reference: http://hyperelliptic.org/EFD/g1p/auto-shortw-jacobian-3.html#addition-add-2007-bl
 */
void P521::addPoint(limb_t *xout, limb_t *yout, limb_t *zout,
                    const limb_t *x1, const limb_t *y1,
                    const limb_t *z1, const limb_t *x2,
                    const limb_t *y2)
{
	limb_t z1z1[NUM_LIMBS_521BIT];
	limb_t u2[NUM_LIMBS_521BIT];
	limb_t s2[NUM_LIMBS_521BIT];
	limb_t h[NUM_LIMBS_521BIT];
	limb_t i[NUM_LIMBS_521BIT];
	limb_t j[NUM_LIMBS_521BIT];
	limb_t r[NUM_LIMBS_521BIT];
	limb_t v[NUM_LIMBS_521BIT];

	// Determine if the first value is the point-at-infinity identity element.
	// The second z value is always 1 so it cannot be the point-at-infinity.
	limb_t p1IsIdentity = BigNumberUtil::isZero(z1, NUM_LIMBS_521BIT);

	// Multiply the points, assuming that z2 = 1.
	square(z1z1, z1);               // z1z1 = z1^2
	mul(u2, x2, z1z1);              // u2 = x2 * z1z1
	mul(s2, y2, z1);                // s2 = y2 * z1 * z1z1
	mul(s2, s2, z1z1);
	sub(h, u2, x1);                 // h = u2 - x1
	mulLiteral(i, h, 2);            // i = (2 * h)^2
	square(i, i);
	sub(r, s2, y1);                 // r = 2 * (s2 - y1)
	add(r, r, r);
	mul(j, h, i);                   // j = h * i
	mul(v, x1, i);                  // v = x1 * i
	square(xout, r);                // xout = r^2 - j - 2 * v
	sub(xout, xout, j);
	sub(xout, xout, v);
	sub(xout, xout, v);
	sub(yout, v, xout);             // yout = r * (v - xout) - 2 * y1 * j
	mul(yout, r, yout);
	mul(j, y1, j);
	sub(yout, yout, j);
	sub(yout, yout, j);
	mul(zout, z1, h);               // zout = 2 * z1 * h
	add(zout, zout, zout);

	// Select the answer to return.  If (x1, y1, z1) was the identity,
	// then the answer is (x2, y2, z2).  Otherwise it is (xout, yout, zout).
	// Conditionally move the second argument over the output if necessary.
	cmove(p1IsIdentity, xout, x2);
	cmove(p1IsIdentity, yout, y2);
	cmove1(p1IsIdentity, zout); // z2 = 1

	// Clean up.
	strict_clean(z1z1);
	strict_clean(u2);
	strict_clean(s2);
	strict_clean(h);
	strict_clean(i);
	strict_clean(j);
	strict_clean(r);
	strict_clean(v);
}

/**
 * \brief Conditionally moves \a y into \a x if a selection value is non-zero.
 *
 * \param select Non-zero to move \a y into \a x, zero to leave \a x unchanged.
 * \param x The destination to move into.
 * \param y The value to conditionally move.
 *
 * The move is performed in a way that it should take the same amount of
 * time irrespective of the value of \a select.
 *
 * \sa cmove1()
 */
void P521::cmove(limb_t select, limb_t *x, const limb_t *y)
{
	uint8_t posn;
	limb_t dummy;
	limb_t sel;

	// Turn "select" into an all-zeroes or all-ones mask.  We don't care
	// which bit or bits is set in the original "select" value.
	sel = (limb_t)(((((dlimb_t)1) << LIMB_BITS) - select) >> LIMB_BITS);
	--sel;

	// Move y into x based on "select".
	for (posn = 0; posn < NUM_LIMBS_521BIT; ++posn) {
		dummy = sel & (*x ^ *y++);
		*x++ ^= dummy;
	}
}

/**
 * \brief Conditionally moves 1 into \a x if a selection value is non-zero.
 *
 * \param select Non-zero to move 1 into \a x, zero to leave \a x unchanged.
 * \param x The destination to move into.
 *
 * The move is performed in a way that it should take the same amount of
 * time irrespective of the value of \a select.
 *
 * \sa cmove()
 */
void P521::cmove1(limb_t select, limb_t *x)
{
	uint8_t posn;
	limb_t dummy;
	limb_t sel;

	// Turn "select" into an all-zeroes or all-ones mask.  We don't care
	// which bit or bits is set in the original "select" value.
	sel = (limb_t)(((((dlimb_t)1) << LIMB_BITS) - select) >> LIMB_BITS);
	--sel;

	// Move 1 into x based on "select".
	dummy = sel & (*x ^ 1);
	*x++ ^= dummy;
	for (posn = 1; posn < NUM_LIMBS_521BIT; ++posn) {
		dummy = sel & *x;
		*x++ ^= dummy;
	}
}

/**
 * \brief Computes the reciprocal of a number modulo 2^521 - 1.
 *
 * \param result The result as a array of NUM_LIMBS_521BIT limbs in size.
 * This cannot be the same array as \a x.
 * \param x The number to compute the reciprocal for, also NUM_LIMBS_521BIT
 * limbs in size.
 */
void P521::recip(limb_t *result, const limb_t *x)
{
	limb_t t1[NUM_LIMBS_521BIT];

	// The reciprocal is the same as x ^ (p - 2) where p = 2^521 - 1.
	// The big-endian hexadecimal expansion of (p - 2) is:
	// 01FF FFFFFFF FFFFFFFF ... FFFFFFFF FFFFFFFD
	//
	// The naive implementation needs to do 2 multiplications per 1 bit and
	// 1 multiplication per 0 bit.  We can improve upon this by creating a
	// pattern 1111 and then shifting and multiplying to create 11111111,
	// and then 1111111111111111, and so on for the top 512-bits.

	// Build a 4-bit pattern 1111 in the result.
	square(result, x);
	mul(result, result, x);
	square(result, result);
	mul(result, result, x);
	square(result, result);
	mul(result, result, x);

	// Shift and multiply by increasing powers of two.  This turns
	// 1111 into 11111111, and then 1111111111111111, and so on.
	for (size_t power = 4; power <= 256; power <<= 1) {
		square(t1, result);
		for (size_t temp = 1; temp < power; ++temp) {
			square(t1, t1);
		}
		mul(result, result, t1);
	}

	// Handle the 9 lowest bits of (p - 2), 111111101, from highest to lowest.
	for (uint8_t index = 0; index < 7; ++index) {
		square(result, result);
		mul(result, result, x);
	}
	square(result, result);
	square(result, result);
	mul(result, result, x);

	// Clean up.
	clean(t1);
}

/**
 * \brief Reduces a number modulo q.
 *
 * \param result The result array, which must be NUM_LIMBS_521BIT limbs in size.
 * \param r The value to reduce, which must be NUM_LIMBS_1042BIT limbs in size.
 *
 * It is allowed for \a result to be the same as \a r.
 */
void P521::reduceQ(limb_t *result, const limb_t *r)
{
	// Algorithm from: http://en.wikipedia.org/wiki/Barrett_reduction
	//
	// We assume that r is less than or equal to (q - 1)^2.
	//
	// We want to compute result = r mod q.  Find the smallest k such
	// that 2^k > q.  In our case, k = 521.  Then set m = floor(4^k / q)
	// and let r = r - q * floor(m * r / 4^k).  This will be the result
	// or it will be at most one subtraction of q away from the result.
	//
	// Note: m is a 522-bit number, which fits in the same number of limbs
	// as a 521-bit number assuming that limbs are 8 bits or more in size.
	static limb_t const numM[NUM_LIMBS_521BIT] PROGMEM = {
		LIMB_PAIR(0x6EC79BF7, 0x449048E1), LIMB_PAIR(0x7663B851, 0xC44A3647),
		LIMB_PAIR(0x08F65A2F, 0x8033FEB7), LIMB_PAIR(0x40D06994, 0xAE79787C),
		LIMB_PAIR(0x00000005, 0x00000000), LIMB_PAIR(0x00000000, 0x00000000),
		LIMB_PAIR(0x00000000, 0x00000000), LIMB_PAIR(0x00000000, 0x00000000),
		LIMB_PARTIAL(0x200)
	};
	limb_t temp[NUM_LIMBS_1042BIT + NUM_LIMBS_521BIT];
	limb_t temp2[NUM_LIMBS_521BIT];

	// Multiply r by m.
	BigNumberUtil::mul_P(temp, r, NUM_LIMBS_1042BIT, numM, NUM_LIMBS_521BIT);

	// Compute (m * r / 4^521) = (m * r / 2^1042).
#if BIGNUMBER_LIMB_8BIT || BIGNUMBER_LIMB_16BIT
	dlimb_t carry = temp[NUM_LIMBS_BITS(1040)] >> 2;
	for (uint8_t index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry += ((dlimb_t)(temp[NUM_LIMBS_BITS(1040) + index + 1])) << (LIMB_BITS - 2);
		temp2[index] = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
#elif BIGNUMBER_LIMB_32BIT || BIGNUMBER_LIMB_64BIT
	dlimb_t carry = temp[NUM_LIMBS_BITS(1024)] >> 18;
	for (uint8_t index = 0; index < NUM_LIMBS_521BIT; ++index) {
		carry += ((dlimb_t)(temp[NUM_LIMBS_BITS(1024) + index + 1])) << (LIMB_BITS - 18);
		temp2[index] = (limb_t)carry;
		carry >>= LIMB_BITS;
	}
#endif

	// Multiply (m * r) / 2^1042 by q and subtract it from r.
	// We can ignore the high words of the subtraction result
	// because they will all turn into zero after the subtraction.
	BigNumberUtil::mul_P(temp, temp2, NUM_LIMBS_521BIT,
	                     P521_q, NUM_LIMBS_521BIT);
	BigNumberUtil::sub(result, r, temp, NUM_LIMBS_521BIT);

	// Perform a trial subtraction of q from the result to reduce it.
	BigNumberUtil::reduceQuick_P(result, result, P521_q, NUM_LIMBS_521BIT);

	// Clean up and exit.
	clean(temp);
	clean(temp2);
}

/**
 * \brief Multiplies two values and then reduces the result modulo q.
 *
 * \param result The result, which must be NUM_LIMBS_521BIT limbs in size
 * and can be the same array as \a x or \a y.
 * \param x The first value to multiply, which must be NUM_LIMBS_521BIT limbs
 * in size and less than q.
 * \param y The second value to multiply, which must be NUM_LIMBS_521BIT limbs
 * in size and less than q.  This can be the same array as \a x.
 */
void P521::mulQ(limb_t *result, const limb_t *x, const limb_t *y)
{
	limb_t temp[NUM_LIMBS_1042BIT];
	mulNoReduce(temp, x, y);
	reduceQ(result, temp);
	strict_clean(temp);
}

/**
 * \brief Computes the reciprocal of a number modulo q.
 *
 * \param result The result as a array of NUM_LIMBS_521BIT limbs in size.
 * This cannot be the same array as \a x.
 * \param x The number to compute the reciprocal for, also NUM_LIMBS_521BIT
 * limbs in size.
 */
void P521::recipQ(limb_t *result, const limb_t *x)
{
	// Bottom 265 bits of q - 2.  The top 256 bits are all-1's.
	static limb_t const P521_q_m2[] PROGMEM = {
		LIMB_PAIR(0x91386407, 0xbb6fb71e), LIMB_PAIR(0x899c47ae, 0x3bb5c9b8),
		LIMB_PAIR(0xf709a5d0, 0x7fcc0148), LIMB_PAIR(0xbf2f966b, 0x51868783),
		LIMB_PARTIAL(0x1fa)
	};

	// Raise x to the power of q - 2, mod q.  We start with the top
	// 256 bits which are all-1's, using a similar technique to recip().
	limb_t t1[NUM_LIMBS_521BIT];
	mulQ(result, x, x);
	mulQ(result, result, x);
	mulQ(result, result, result);
	mulQ(result, result, x);
	mulQ(result, result, result);
	mulQ(result, result, x);
	for (size_t power = 4; power <= 128; power <<= 1) {
		mulQ(t1, result, result);
		for (size_t temp = 1; temp < power; ++temp) {
			mulQ(t1, t1, t1);
		}
		mulQ(result, result, t1);
	}
	clean(t1);

	// Deal with the bottom 265 bits from highest to lowest.  Square for
	// each bit and multiply in x whenever there is a 1 bit.  The timing
	// is based on the publicly-known constant q - 2, not on the value of x.
	size_t bit = 265;
	while (bit > 0) {
		--bit;
		mulQ(result, result, result);
		if (pgm_read_limb(&(P521_q_m2[bit / LIMB_BITS])) &
		        (((limb_t)1) << (bit % LIMB_BITS))) {
			mulQ(result, result, x);
		}
	}
}

/**
 * \brief Generates a k value using the algorithm from RFC 6979.
 *
 * \param k The value to generate.
 * \param hm The hashed message formatted ready to be signed.
 * \param x The private key to sign with.
 * \param hash The hash algorithm to use.
 * \param count Iteration counter for generating new values of k when the
 * previous one is rejected.
 */
void P521::generateK(uint8_t k[66], const uint8_t hm[66],
                     const uint8_t x[66], Hash *hash, uint64_t count)
{
	size_t hlen = hash->hashSize();
	uint8_t V[64];
	uint8_t K[64];
	uint8_t marker;

	// If for some reason a hash function was supplied with more than
	// 512 bits of output, truncate hash values to the first 512 bits.
	// We cannot support more than this yet.
	if (hlen > 64) {
		hlen = 64;
	}

	// RFC 6979, Section 3.2, Step a.  Hash the message, reduce modulo q,
	// and produce an octet string the same length as q, bits2octets(H(m)).
	// We support hashes up to 512 bits and q is a 521-bit number, so "hm"
	// is already the bits2octets(H(m)) value that we need.

	// Steps b and c.  Set V to all-ones and K to all-zeroes.
	memset(V, 0x01, hlen);
	memset(K, 0x00, hlen);

	// Step d.  K = HMAC_K(V || 0x00 || x || hm).  We make a small
	// modification here to append the count value if it is non-zero.
	// We use this to generate a new k if we have to re-enter this
	// function because the previous one was rejected by sign().
	// This is slightly different to RFC 6979 which says that the
	// loop in step h below should be continued.  That code path is
	// difficult to access, so instead modify K and V in steps d and f.
	// This alternative construction is compatible with the second
	// variant described in section 3.6 of RFC 6979.
	hash->resetHMAC(K, hlen);
	hash->update(V, hlen);
	marker = 0x00;
	hash->update(&marker, 1);
	hash->update(x, 66);
	hash->update(hm, 66);
	if (count) {
		hash->update(&count, sizeof(count));
	}
	hash->finalizeHMAC(K, hlen, K, hlen);

	// Step e.  V = HMAC_K(V)
	hash->resetHMAC(K, hlen);
	hash->update(V, hlen);
	hash->finalizeHMAC(K, hlen, V, hlen);

	// Step f.  K = HMAC_K(V || 0x01 || x || hm)
	hash->resetHMAC(K, hlen);
	hash->update(V, hlen);
	marker = 0x01;
	hash->update(&marker, 1);
	hash->update(x, 66);
	hash->update(hm, 66);
	if (count) {
		hash->update(&count, sizeof(count));
	}
	hash->finalizeHMAC(K, hlen, K, hlen);

	// Step g.  V = HMAC_K(V)
	hash->resetHMAC(K, hlen);
	hash->update(V, hlen);
	hash->finalizeHMAC(K, hlen, V, hlen);

	// Step h.  Generate candidate k values until we find what we want.
	for (;;) {
		// Step h.1 and h.2.  Generate a string of 66 bytes in length.
		//      T = empty
		//      while (len(T) < 66)
		//          V = HMAC_K(V)
		//          T = T || V
		size_t posn = 0;
		while (posn < 66) {
			size_t temp = 66 - posn;
			if (temp > hlen) {
				temp = hlen;
			}
			hash->resetHMAC(K, hlen);
			hash->update(V, hlen);
			hash->finalizeHMAC(K, hlen, V, hlen);
			memcpy(k + posn, V, temp);
			posn += temp;
		}

		// Step h.3.  k = bits2int(T) and exit the loop if k is not in
		// the range 1 to q - 1.  Note: We have to extract the 521 most
		// significant bits of T, which means shifting it right by seven
		// bits to put it into the correct form.
		for (posn = 65; posn > 0; --posn) {
			k[posn] = (k[posn - 1] << 1) | (k[posn] >> 7);
		}
		k[0] >>= 7;
		if (isValidPrivateKey(k)) {
			break;
		}

		// Generate new K and V values and try again.
		//      K = HMAC_K(V || 0x00)
		//      V = HMAC_K(V)
		hash->resetHMAC(K, hlen);
		hash->update(V, hlen);
		marker = 0x00;
		hash->update(&marker, 1);
		hash->finalizeHMAC(K, hlen, K, hlen);
		hash->resetHMAC(K, hlen);
		hash->update(V, hlen);
		hash->finalizeHMAC(K, hlen, V, hlen);
	}

	// Clean up.
	clean(V);
	clean(K);
}

/**
 * \brief Generates a k value using the algorithm from RFC 6979.
 *
 * \param k The value to generate.
 * \param hm The hashed message formatted ready to be signed.
 * \param x The private key to sign with.
 * \param count Iteration counter for generating new values of k when the
 * previous one is rejected.
 *
 * This override uses SHA512 to generate k values.  It is used when
 * sign() was not passed an explicit hash object by the application.
 */
void P521::generateK(uint8_t k[66], const uint8_t hm[66],
                     const uint8_t x[66], uint64_t count)
{
	SHA512 hash;
	generateK(k, hm, x, &hash, count);
}
