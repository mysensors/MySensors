/*
 * RemoteSwitch library v2.3.0 (20121229) made by Randy Simons http://randysimons.nl/
 * See RemoteReceiver.h for details.
 *
 * License: GPLv3. See license.txt
 */

#include "RemoteReceiver.h"


/************
* RemoteReceiver
************/

int8_t RemoteReceiver::_interrupt;
volatile int8_t RemoteReceiver::_state;
byte RemoteReceiver::_minRepeats;
RemoteReceiverCallBack RemoteReceiver::_callback;
boolean RemoteReceiver::_inCallback = false;
boolean RemoteReceiver::_enabled = false;

void RemoteReceiver::init(int8_t interrupt, byte minRepeats, RemoteReceiverCallBack callback) {
	_interrupt = interrupt;
	_minRepeats = minRepeats;
	_callback = callback;

	enable();
	if (_interrupt >= 0) {
		attachInterrupt(_interrupt, interruptHandler, CHANGE);
	}
}

void RemoteReceiver::enable() {
	_state = -1;
	_enabled = true;
}

void RemoteReceiver::disable() {
	_enabled = false;
}

void RemoteReceiver::deinit() {
	_enabled = false;
	if (_interrupt >= 0) {
		detachInterrupt(_interrupt);
	}
}

void RemoteReceiver::interruptHandler() {
	if (!_enabled) {
		return;
	}

	static unsigned int period;				// Calculated duration of 1 period
	static byte receivedBit;				// Contains "bit" currently receiving
	static unsigned long receivedCode;		// Contains received code
	static unsigned long previousCode;		// Contains previous received code
	static byte repeats = 0;				// The number of times the an identical code is received in a row.
	static unsigned long edgeTimeStamp[3] = {0, };	// Timestamp of edges
	static unsigned int min1Period, max1Period, min3Period, max3Period;
	static bool skip;

	// Filter out too short pulses. This method works as a low pass filter.
	edgeTimeStamp[1] = edgeTimeStamp[2];
	edgeTimeStamp[2] = micros();

	if (skip) {
		skip = false;
		return;
	}

	if (_state >= 0 && edgeTimeStamp[2]-edgeTimeStamp[1] < min1Period) {
		// Last edge was too short.
		// Skip this edge, and the next too.
		skip = true;
		return;
	}

	unsigned int duration = edgeTimeStamp[1] - edgeTimeStamp[0];
	edgeTimeStamp[0] = edgeTimeStamp[1];

	// Note that if state>=0, duration is always >= 1 period.

	if (_state==-1) { // Waiting for sync-signal
		if (duration>3720) { // =31*120 minimal time between two edges before decoding starts.
			// Sync signal received.. Preparing for decoding
			period=duration/31;
			receivedCode=previousCode=repeats=0;

			// Allow for large error-margin. ElCheapo-hardware :(
			min1Period=period*4/10; // Avoid floating point math; saves memory.
			max1Period=period*16/10;
			min3Period=period*23/10;
			max3Period=period*37/10;
		}
		else {
			return;
		}
	} else if (_state<48) { // Decoding message
		receivedBit <<= 1;

		// bit part durations can ONLY be 1 or 3 periods.
		if (duration<=max1Period) {
			receivedBit &= B1110; // Clear LSB of receivedBit
		}
		else if (duration>=min3Period && duration<=max3Period) {
			receivedBit |= B1; // Set LSB of receivedBit
		}
		else { // Otherwise the entire sequence is invalid
			_state=-1;
			return;
		}

		if ((_state%4)==3) { // Last bit part?
			// Shift
			receivedCode*=3;

			// Only 4 LSB's are used; trim the rest.
			switch (receivedBit & B1111) {
				case B0101: // short long short long == B0101
					// bit "0" received
					receivedCode+=0; // I hope the optimizer handles this ;)
					break;
				case B1010: // long short long short == B1010
					// bit "1" received
					receivedCode+=1;
					break;
				case B0110: // short long long short
					// bit "f" received
					receivedCode+=2;
					break;
				default:
					// Bit was rubbish. Abort.
					_state=-1;
					return;
			}
		}
	} else if (_state==48) { // Waiting for sync bit part 1
		// Must be 1 period.
		if (duration>max1Period) {
			_state=-1;
			return;
		}
	} else { // Waiting for sync bit part 2
		// Must be 31 periods.
		if (duration<period*25 || duration>period*36) {
		  _state=-1;
		  return;
		}

		// receivedCode is a valid code!

		if (receivedCode!=previousCode) {
			repeats=0;
			previousCode=receivedCode;
		}

		repeats++;

		if (repeats>=_minRepeats) {
			if (!_inCallback) {
				_inCallback = true;
				(_callback)(receivedCode, period);
				_inCallback = false;
			}
			// Reset after callback.
			_state=-1;
			return;
		}

		// Reset for next round
		receivedCode = 0;
		_state=0; // no need to wait for another sync-bit!
		return;
	}

	_state++;
	return;
}

boolean RemoteReceiver::isReceiving(int waitMillis) {
	unsigned long startTime=millis();

	int waited; // Signed int!
	do {
		if (_state == 48) { // Abort if a valid code has been received in the mean time
			return true;
		}
		waited = (millis()-startTime);
	} while(waited>=0 && waited <= waitMillis); // Yes, clock wraps every 50 days. And then you'd have to wait for a looooong time.

	return false;
}
