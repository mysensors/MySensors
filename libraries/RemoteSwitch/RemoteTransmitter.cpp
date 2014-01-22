/*
 * RemoteSwitch library v2.3.0 (20121229) made by Randy Simons http://randysimons.nl/
 * See RemoteTransmitter.h for details.
 *
 * License: GPLv3. See license.txt
 */

#include "RemoteTransmitter.h"


/************
* RemoteTransmitter
************/

RemoteTransmitter::RemoteTransmitter(byte pin, unsigned int periodusec, byte repeats) {
	_pin=pin;
	_periodusec=periodusec;
	_repeats=repeats;

	pinMode(_pin, OUTPUT);
}

unsigned long RemoteTransmitter::encodeTelegram(byte trits[]) {
	unsigned long data = 0;

	// Encode data
	for (byte i=0;i<12;i++) {
		data*=3;
		data+=trits[i];
	}

	// Encode period duration
	data |= (unsigned long)_periodusec << 23;

	// Encode repeats
	data |= (unsigned long)_repeats << 20;

	return data;
}

void RemoteTransmitter::sendTelegram(byte trits[]) {
	sendTelegram(encodeTelegram(trits),_pin);
}

/**
* Format data:
* pppppppp|prrrdddd|dddddddd|dddddddd (32 bit)
* p = perioud (9 bit unsigned int
* r = repeats as 2log. Thus, if r = 3, then signal is sent 2^3=8 times
* d = data
*/
void RemoteTransmitter::sendTelegram(unsigned long data, byte pin) {
	unsigned int periodusec = (unsigned long)data >> 23;
	byte repeats = ((unsigned long)data >> 20) & B111;

	sendCode(pin, data, periodusec, repeats);
}

void RemoteTransmitter::sendCode(byte pin, unsigned long code, unsigned int periodusec, byte repeats) {
	code &= 0xfffff; // Truncate to 20 bit ;
	// Convert the base3-code to base4, to avoid lengthy calculations when transmitting.. Messes op timings.
	// Also note this swaps endianess in the process. The MSB must be transmitted first, but is converted to
	// LSB here. This is easier when actually transmitting later on.
	unsigned long dataBase4 = 0;

	for (byte i=0; i<12; i++) {
		dataBase4<<=2;
		dataBase4|=(code%3);
		code/=3;
	}

	repeats = 1 << (repeats & B111); // repeats := 2^repeats;

	for (byte j=0;j<repeats;j++) {
		// Sent one telegram

		// Recycle code as working var to save memory
		code=dataBase4;
		for (byte i=0; i<12; i++) {
			switch (code & B11) {
				case 0:
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec*3);
					break;
				case 1:
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec);
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec);
					break;
				case 2: // KA: X or float
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, HIGH);
					delayMicroseconds(periodusec*3);
					digitalWrite(pin, LOW);
					delayMicroseconds(periodusec);
					break;
			}
			// Next trit
			code>>=2;
		}

		// Send termination/synchronization-signal. Total length: 32 periods
		digitalWrite(pin, HIGH);
		delayMicroseconds(periodusec);
		digitalWrite(pin, LOW);
		delayMicroseconds(periodusec*31);
	}
}

boolean RemoteTransmitter::isSameCode(unsigned long encodedTelegram, unsigned long receivedData) {
	return (receivedData==(encodedTelegram & 0xFFFFF)); // compare the 20 LSB's
}


/************
* ActionTransmitter
************/

ActionTransmitter::ActionTransmitter(byte pin, unsigned int periodusec, byte repeats) : RemoteTransmitter(pin,periodusec,repeats) {
	// Call constructor
}


void ActionTransmitter::sendSignal(byte systemCode, char device, boolean on) {
	sendTelegram(getTelegram(systemCode,device,on), _pin);
}

unsigned long ActionTransmitter::getTelegram(byte systemCode, char device, boolean on) {
	byte trits[12];

	device-=65;

	for (byte i=0; i<5; i++) {
		// Trits 0-4 contain address (2^5=32 addresses)
		trits[i]=(systemCode & 1)?1:2;
		systemCode>>=1;

		// Trits 5-9 contain device. Only one trit has value 0, others have 2 (float)!
		trits[i+5]=(i==device?0:2);
	}

	// Switch on or off
	trits[10]=(!on?0:2);
	trits[11]=(on?0:2);

	return encodeTelegram(trits);
}

/************
* BlokkerTransmitter
************/

BlokkerTransmitter::BlokkerTransmitter(byte pin, unsigned int periodusec, byte repeats) : RemoteTransmitter(pin,periodusec,repeats) {
	// Call constructor
}


void BlokkerTransmitter::sendSignal(byte device, boolean on) {
	sendTelegram(getTelegram(device,on), _pin);
}

unsigned long BlokkerTransmitter::getTelegram(byte device, boolean on) {
	byte trits[12]={0};

	device--;

	for (byte i=1; i<4; i++) {
		// Trits 1-3 contain device
		trits[i]=(device & 1)?0:1;
		device>>=1;
	}

	// Switch on or off
	trits[8]=(on?1:0);

	return encodeTelegram(trits);
}

/************
* KaKuTransmitter
************/

KaKuTransmitter::KaKuTransmitter(byte pin, unsigned int periodusec, byte repeats) : RemoteTransmitter(pin,periodusec,repeats) {
	// Call constructor
}

void KaKuTransmitter::sendSignal(char address, byte device, boolean on) {
	sendTelegram(getTelegram(address, device, on), _pin);
}

unsigned long KaKuTransmitter::getTelegram(char address, byte device, boolean on) {
	byte trits[12];

	address-=65;
	device-=1;

	for (byte i=0; i<4; i++) {
		// Trits 0-3 contain address (2^4 = 16 addresses)
		trits[i]=(address & 1)?2:0;
		address>>=1;

		// Trits 4-8 contain device (2^4 = 16 addresses)
		trits[i+4]=(device & 1)?2:0;
		device>>=1;
	}

	// Trits 8-10 seem to be fixed
	trits[8]=0;
	trits[9]=2;
	trits[10]=2;

	// Switch on or off
	trits[11]=(on?2:0);

	return encodeTelegram(trits);
}

void KaKuTransmitter::sendSignal(char address, byte group, byte device, boolean on) {
	sendTelegram(getTelegram(address, group, on), _pin);
}

unsigned long KaKuTransmitter::getTelegram(char address, byte group, byte device, boolean on) {
	byte trits[12], i;

	address-=65;
	group-=1;
	device-=1;

	// Address. M3E Pin A0-A3
	for (i=0; i<4; i++) {
		// Trits 0-3 contain address (2^4 = 16 addresses)
		trits[i]=(address & 1)?2:0;
		address>>=1;
	}

	// Device. M3E Pin A4-A5
	for (; i<6; i++) {
		trits[i]=(device & 1)?2:0;
		device>>=1;
	}

	// Group. M3E Pin A6-A7
	for (; i<8; i++) {
		trits[i]=(group & 1)?2:0;
		group>>=1;
	}

	// Trits 8-10 are be fixed. M3E Pin A8/D0-A10/D2
	trits[8]=0;
	trits[9]=2;
	trits[10]=2;

	// Switch on or off, M3E Pin A11/D3
	trits[11]=(on?2:0);

	return encodeTelegram(trits);
}


/************
* ElroTransmitter
************/

ElroTransmitter::ElroTransmitter(byte pin, unsigned int periodusec, byte repeats) : RemoteTransmitter(pin, periodusec, repeats) {
	//Call constructor
}

void ElroTransmitter::sendSignal(byte systemCode, char device, boolean on) {
	sendTelegram(getTelegram(systemCode, device, on), _pin);
}

unsigned long ElroTransmitter::getTelegram(byte systemCode, char device, boolean on) {
	byte trits[12];

	device-=65;

	for (byte i=0; i<5; i++) {
		//trits 0-4 contain address (2^5=32 addresses)
		trits[i]=(systemCode & 1)?0:2;
		systemCode>>=1;

		//trits 5-9 contain device. Only one trit has value 0, others have 2 (float)!
		trits[i+5]=(i==device?0:2);
	}

	//switch on or off
	trits[10]=(on?0:2);
	trits[11]=(!on?0:2);

	return encodeTelegram(trits);
}
