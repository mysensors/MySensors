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

#include "NoiseSource.h"
#include "RNG.h"

/**
 * \class NoiseSource NoiseSource.h <NoiseSource.h>
 * \brief Abstract base class for random noise sources.
 *
 * \sa \link RNGClass RNG\endlink, TransistorNoiseSource
 */

/**
 * \brief Constructs a new random noise source.
 */
NoiseSource::NoiseSource()
{
}

/**
 * \brief Destroys this random noise source.
 */
NoiseSource::~NoiseSource()
{
}

/**
 * \fn bool NoiseSource::calibrating() const
 * \brief Determine if the noise source is still calibrating itself.
 *
 * \return Returns true if calibration is in progress; false if the noise
 * source is generating valid random data.
 *
 * Noise sources that require calibration start doing so at system startup
 * and then switch over to random data generation once calibration is complete.
 * Since no random data is being generated during calibration, the output
 * from \link RNGClass::rand() RNG.rand()\endlink may be predictable.
 * Use \link RNGClass::available() RNG.available()\endlink to determine
 * when sufficient entropy is available to generate good random values.
 *
 * It is possible that the noise source never exits calibration.  This can
 * happen if the input voltage is insufficient to trigger noise or if the
 * noise source is not connected.  Noise sources may also periodically
 * recalibrate themselves.
 *
 * \sa stir()
 */

/**
 * \fn void NoiseSource::stir()
 * \brief Stirs entropy from this noise source into the global random
 * number pool.
 *
 * This function should call output() to add the entropy from this noise
 * source to the global random number pool.
 *
 * The noise source should batch up the entropy data, providing between
 * 16 and 48 bytes of data each time.  If the noise source does not have
 * sufficient entropy data at the moment, it should return without stiring
 * the current data in.
 *
 * \sa calibrating(), output()
 */

/**
 * \brief Called when the noise source is added to RNG with
 * \link RNGClass::addNoiseSource() RNG.addNoiseSource()\endlink.
 *
 * This function is intended for noise source initialization tasks that
 * must be performed after \link RNGClass::begin() RNG.begin()\endlink
 * has been called to initialize the global random number pool.
 * For example, if the noise source has a unique identifier or serial
 * number then this function can stir it into the pool at startup time.
 */
void NoiseSource::added()
{
	// Nothing to do here.
}

/**
 * \brief Called from subclasses to output noise to the global random
 * number pool.
 *
 * \param data Points to the noise data.
 * \param len Number of bytes of noise data.
 * \param credit The number of bits of entropy to credit for the data.
 * Note that this is bits, not bytes.
 *
 * The default implementation of this function calls
 * \link RNGClass::stir() RNG.stir()\endlink to add the entropy from
 * this noise source to the global random number pool.
 *
 * This function may be overridden by subclasses to capture the raw
 * output from the noise source before it is mixed into the pool to
 * allow the raw data to be analyzed for randomness.
 */
void NoiseSource::output(const uint8_t *data, size_t len, unsigned int credit)
{
	RNG.stir(data, len, credit);
}
