/*
 * RemoteSensor library v1.0.2 (20130601) for Arduino 1.0
 * 
 * This library receives, decodes, decrypts and receives data of
 * remote weather sensors made by Hideki Electronics.
 * 
 * Copyright 2011-2013 by Randy Simons http://randysimons.nl/
 *
 * Parts of this code based on Oopsje's CrestaProtocol.pdf, for which
 * I thank him very much!
 *
 * For more details about the data format, see CrestaProtocol.pdf
 * 
 * License: GPLv3. See license.txt
 */

#ifndef SensorReceiver_h
#define SensorReceiver_h

#include <Arduino.h>

typedef void (*SensorReceiverCallback)(byte *); // pointer to data

/**
 * Generic class for receiving and decoding 433MHz remote weather sensors as made by Cresta.
 * E.g. http://www.cresta.nl/index.php?Itemid=2&option=com_zoo&view=item&category_id=32&item_id=281&lang=en
 *
 * Cresta is just a brandname. The original OEM seems to be Hideki Electronics. There are
 * other brands which use the same hardware and / or protocol. As far as I know these include
 * Mebus, Irox, Honeywell, Cresta and RST.
 *
 *
 * This class should be able to receive all sensor types: thermo/hygro, rain, uv, anemo.
 * However, only thermo/hygro is tested and has special support.
 *
 * Hardware required for this library:
 * A 433MHz/434MHz SAW receiver, e.g. http://www.sparkfun.com/products/10532
 */
class SensorReceiver {
	public:
		/**
		 * Initializes the receiver. When a valid data package has been received, the callback is called
		 * with a pointer to the validated and decrypted data. For more details about the data format,
		 * see CrestaProtocol.pdf
		 *
		 * For the thermo/hygro-sensor, you can use decodeThermoHygro() for easy decoding the data.
		 *
		 * If interrupt >= 0, init will register pin <interrupt> to this library.
		 * If interrupt < 0, no interrupt is registered. In that case, you have to call interruptHandler()
		 * yourself whenever the output of the receiver changes, or you can use InterruptChain.
		 *
		 * @param interrupt 	The interrupt as is used by Arduino's attachInterrupt function. See attachInterrupt for details.
								If < 0, you must call interruptHandler() yourself.
		 * @param callbackIn	Pointer to a callback function, with signature void (*func)(byte *, byte). 
		 *						First parameter is the decoded data, the second the length of the package in bytes, including checksums.
		 * 					
		 */
		static void init(int8_t interrupt, SensorReceiverCallback callbackIn);

		/**
		* Decodes data of a Thermo Hygro sensor. Note that the unit of the temp is in dec-degree, or degrees times 10.
		* Thus, a value of temp of 235 is actually 23.5 degrees.
		*/
		static void decodeThermoHygro(byte *data, byte &channel, byte &randomId, int &temp, byte &humidity);

		/**
		* Enable decoding. No need to call enable() after init().
		*/
		static void enable();

		/**
		* Disable decoding. You can re-enable decoding by calling enable();
		*/
		static void disable();
		
		/**
		 * interruptHandler is called on every change in the input signal. If SensorReceiver::init is called
		 * with interrupt <0, you have to call interruptHandler() yourself. (Or use InterruptChain)
		 */
		static void interruptHandler();
     
	private:
		/**
		 * Quasi-reset. Called when the current edge is too long or short.
		 * reset "promotes" the current edge as being the first edge of a new sequence.
		 */
		static void reset();

		/**
		* Internal functions, based on CrestaProtocol.pdf
		*/
		static boolean decryptAndCheck();
		static byte secondCheck(byte b);

		static byte halfBit; 				// 9 bytes of 9 bits each, 2 edges per bit = 162 halfbits for thermo/hygro
		static word clockTime; 				// Measured duration of half a period, i.e. the the duration of a short edge.
		static boolean isOne;    			// true if the the last bit is a logic 1.
		static unsigned long lastChange;	// Timestamp of previous edge
		static SensorReceiverCallback callback;	// Pointer to callback function, which is called after valid package has been received
		static byte packageLength;
		static word duration;				// Duration of current edge.
		static boolean enabled;				// If true, monitoring and decoding is enabled. If false, interruptHandler will return immediately.

		static byte data[14]; 				// Maximum number of bytes used by Cresta
};

#endif