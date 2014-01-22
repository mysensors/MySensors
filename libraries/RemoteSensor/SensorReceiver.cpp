/*
 * RemoteSensor library v1.0.2 (20130601) for Arduino 1.0
 *
 * This library encodes, encrypts en transmits data to
 * remote weather stations made by Hideki Electronics..
 * 
 * Copyright 20112-2013 by Randy Simons http://randysimons.nl/
 *
 * Parts of this code based on Oopsje's CrestaProtocol.pdf, for which
 * I thank him very much!
 * 
 * License: GPLv3. See license.txt
 */

#include <SensorReceiver.h>

byte SensorReceiver::halfBit = 0;
word SensorReceiver::clockTime;
boolean SensorReceiver::isOne;
unsigned long SensorReceiver::lastChange=0;
SensorReceiverCallback SensorReceiver::callback;
byte SensorReceiver::data[14];
byte SensorReceiver::packageLength;
word SensorReceiver::duration;
boolean SensorReceiver::enabled;

void SensorReceiver::init(int8_t interrupt, SensorReceiverCallback callbackIn) {
  callback = callbackIn;
  
  enable();
  
  if (interrupt >= 0) {
	attachInterrupt(interrupt, interruptHandler, CHANGE);
  }  
}

void SensorReceiver::interruptHandler() {	
	if (!enabled) {
		return;
	}
	
	/* I'll follow CrestaProtocol documentation here. However, I suspect it is inaccurate at some points:
	* - there is no stop-bit after every byte. Instead, there's a start-bit (0) before every byte.
	* - Conversely, there is no start-bit "1" before every byte.
	* - An up-flank is 0, down-flank is 1, at least with both my receivers.
	*
	* However, since the first start-bit 0 is hard to distinguish given the current clock-detecting
	* algorithm, I pretend there *is* a stop-bit 0 instead of start-bit. However, this means the
	* last stop-bit of a package must be ignored, as it simply isn't there.
	*
	* This manchester decoder is based on the principle that short edges indicate the current bit is the
	* same as previous bit, and that long edge indicate that the current bit is the complement of the
	* previous bit.
	*/
	
	static byte halfBitCounter = 255;
	unsigned long currentTime=micros();
	duration=currentTime-lastChange; // Duration = Time between edges

	lastChange=currentTime;

	if (halfBit==0) {
		// Automatic clock detection. One clock-period is half the duration of the first edge.
		clockTime = duration >> 1;
		
		// Some sanity checking, very short (<200us) or very long (>1000us) signals are ignored.
		if (clockTime < 200 || clockTime > 1000) {
			return;
		}
		isOne = true;
	} 
	else {
		// Edge is not too long, nor too short?
		if (duration < (clockTime >> 1) || duration > (clockTime << 1) + clockTime) { // read as: duration < 0.5 * clockTime || duration > 3 * clockTime
			// Fail. Abort.
			reset();
			return;
		}

		// Only process every second half bit, i.e. every whole bit.
		if (halfBit & 1) {  
			byte currentByte = halfBit / 18;
			byte currentBit = (halfBit >> 1) % 9; // nine bits in a byte.

			if (currentBit < 8) {
				if (isOne) {
					// Set current bit of current byte
					data[currentByte] |= 1 << currentBit;
				} 
				else {
				  // Reset current bit of current byte
				  data[currentByte] &= ~(1 << currentBit);
				}
			} 
			else {
				// Ninth bit must be 0
				if (isOne) {
					// Bit is 1. Fail. Abort.
					reset();
					return;
				}                    
			}

			if (halfBit == 17) { // First byte has been received
				// First data byte must be x75.
				if (data[0] != 0x75) {
					reset();
					return;
				}
			} 
			else if (halfBit == 53) { // Third byte has been received
				// Obtain the length of the data
				byte decodedByte = data[2]^(data[2]<<1);
				packageLength = (decodedByte >> 1) & 0x1f;

				// Do some checking to see if we should proceed
				if (packageLength < 6 || packageLength > 11) {
					reset();
					return;
				}

				halfBitCounter = (packageLength + 3) * 9 * 2 - 2 - 1; // 9 bits per byte, 2 edges per bit, minus last stop-bit (see comment above)
			}

			// Done?
			if (halfBit >= halfBitCounter) {
				if (halfBit == halfBitCounter) {
					// Yes! Decrypt and call the callback					
					if (decryptAndCheck()) {
						(callback)(data);
					}
				}

				// reset
				halfBit = 0;
				return;
			}
		}

		// Edge is long?
		if (duration > clockTime + (clockTime >> 1)) { // read as: duration > 1.5 * clockTime
			// Long edge.
			isOne = !isOne;
			// Long edge takes 2 halfbits
			halfBit++;
		}
	}

	halfBit++;
	return;
}

void SensorReceiver::reset() {
	halfBit = 1;
	clockTime = duration >> 1;
	isOne = true;
}

boolean SensorReceiver::decryptAndCheck() { 
	byte cs1,cs2,i; 

	cs1=0; 
	cs2=0; 
	for (i=1; i<packageLength+2; i++) { 
		cs1^=data[i]; 
		cs2 = secondCheck(data[i]^cs2); 
		data[i] ^= data[i] << 1; 
	} 

	if (cs1) {
		return false; 
	}

	if (cs2 != data[packageLength+2]) {
		return false;
	}
	return true; 
}

byte SensorReceiver::secondCheck(byte b) { 
  byte c;
  
	if (b&0x80)
		b^=0x95; 
	c = b^(b>>1); 
	if (b&1)
		c^=0x5f; 
	if (c&1)
		b^=0x5f;
    
  return b^(c>>1); 
}

void SensorReceiver::enable() {
	halfBit = 0;
	enabled = true;
}

void SensorReceiver::disable() {
	enabled = false;
}

void SensorReceiver::decodeThermoHygro(byte *data, byte &channel, byte &randomId, int &temp, byte &humidity) {
	channel = data[1] >> 5;
	
	// Internally channel 4 is used for the other sensor types (rain, uv, anemo).
	// Therefore, if channel is decoded 5 or 6, the real value set on the device itself is 4 resp 5.
	if (channel >= 5) {
		channel--;
	}
	
	randomId = data[1] & 0x1f;
	
	temp = 100 * (data[5] & 0x0f) + 10 * (data[4] >> 4) + (data[4] & 0x0f);
	// temp is negative?
	if (!(data[5] & 0x80)) {
		temp = -temp;
	}
	
	humidity = 10 * (data[6] >> 4) + (data[6] & 0x0f);
}