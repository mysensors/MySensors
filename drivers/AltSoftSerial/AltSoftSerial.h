/* An Alternative Software Serial Library
 * http://www.pjrc.com/teensy/td_libs_AltSoftSerial.html
 * Copyright (c) 2014 PJRC.COM, LLC, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef AltSoftSerial_h
#define AltSoftSerial_h

#include <inttypes.h>

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#include "pins_arduino.h"
#endif

#if defined(__arm__) && defined(CORE_TEENSY)
#define ALTSS_BASE_FREQ F_BUS
#else
#define ALTSS_BASE_FREQ F_CPU
#endif

/** AltSoftSerial class */
class AltSoftSerial : public Stream
{
public:
	AltSoftSerial() { } //!< Constructor
	~AltSoftSerial()
	{
		end(); //!< Destructor
	}
	static void begin(uint32_t baud)
	{
		init((ALTSS_BASE_FREQ + baud / 2) / baud); //!< begin
	}
	static void end(); //!< end
	int peek(); //!< peek
	int read(); //!< read
	int available(); //!< available
#if ARDUINO >= 100
	size_t write(uint8_t byte)
	{
		writeByte(byte); //!< write
		return 1;
	}
	void flush()
	{
		flushOutput(); //!< flush
	}
#else
	void write(uint8_t byte)
	{
		writeByte(byte); //!< write
	}
	void flush()
	{
		flushInput(); //!< flush
	}
#endif
	using Print::write;
	static void flushInput(); //!< flushInput
	static void flushOutput(); //!< flushOutput
	// for drop-in compatibility with NewSoftSerial, rxPin & txPin ignored
	AltSoftSerial(uint8_t rxPin, uint8_t txPin, bool inverse = false)
	{
		(void)rxPin; //!< AltSoftSerial
		(void)txPin;
		(void)inverse;
	}
	bool listen()
	{
		return false; //!< listen
	}
	bool isListening()
	{
		return true; //!< isListening
	}
	bool overflow()
	{
		bool r = timing_error; //!< overflow
		timing_error = false;
		return r;
	}
	static int library_version()
	{
		return 1; //!< library_version
	}
	static void enable_timer0(bool enable)
	{
		(void)enable; //!< enable_timer0
	}
	static bool timing_error; //!< timing_error
private:
	static void init(uint32_t cycles_per_bit);
	static void writeByte(uint8_t byte);
};

#endif
