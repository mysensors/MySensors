/*
 * RemoteSensor library v1.0.2 (20130601) for Arduino 1.0
 *
 * This library encodes, encrypts en transmits data to
 * remote weather stations made by Hideki Electronics..
 * 
 * Copyright 2011-2013 by Randy Simons http://randysimons.nl/
 *
 * Parts of this code based on Oopsje's CrestaProtocol.pdf, for which
 * I thank him very much!
 * 
 * License: GPLv3. See license.txt
 */

#include <SensorTransmitter.h>



/*******************
 * Sensor base class
 ******************/
 
SensorTransmitter::SensorTransmitter(byte transmitterPin, byte randomId) {
	_transmitterPin = transmitterPin;
	_randomId = randomId;
	
	pinMode(_transmitterPin, OUTPUT);
}

/* Encrypt data byte to send to station */
byte SensorTransmitter::encryptByte(byte b) { 
	byte a; 
	for(a=0; b; b<<=1) {
		a^=b;
	} 
	return a; 
}

/* The second checksum. Input is OldChecksum^NewByte */
byte SensorTransmitter::secondCheck(byte b) { 
	byte c; 
	if (b&0x80) {
		b^=0x95;
	}
	c = b^(b>>1); 
	if (b&1) {
		c^=0x5f;
	} 
	if (c&1) {
		b^=0x5f; 
	}
	return b^(c>>1); 
} 

/* Example to encrypt a package for sending, 
 Input: Buffer holds the unencrypted data. 
 Returns the number of bytes to send, 
 Buffer now holds data ready for sending. 
 */
byte SensorTransmitter::encryptAndAddCheck(byte *buffer) { 
	byte cs1,cs2,count,i; 

	count=(buffer[2]>>1) & 0x1f; 
	cs1=0; 
	cs2=0; 
	for(i=1; i<count+1; i++) { 
		buffer[i]=encryptByte(buffer[i]); 
		cs1^=buffer[i]; 
		cs2 =secondCheck(buffer[i]^cs2); 
	} 
	buffer[count+1]=cs1; 
	buffer[count+2]=secondCheck(cs1^cs2); 
	return count+3; 
} 

/* Send one byte and keep the transmitter ready to send the next */
void SensorTransmitter::sendManchesterByte(byte transmitterPin, byte b) {
	byte i;

	// Send start-bit 0.  
	digitalWrite(transmitterPin, LOW);
	delayMicroseconds(500);
	digitalWrite(transmitterPin, HIGH);
	delayMicroseconds(500);

	for (i = 0; i < 16; i++) {
		if (b&1) {
			digitalWrite(transmitterPin, HIGH);
		} else {
			digitalWrite(transmitterPin, LOW);
		}

		delayMicroseconds(500);  /* 500uS delay */

		b=~b; 
		if (i&1) {
			b>>=1;
		}
	} 
}

/* Send bytes (prepared by “encryptAndAddCheck”) and pause at the end. */
void SensorTransmitter::sendManchesterPackage(byte transmitterPin, byte *data, byte cnt) {
	byte i;

	for (i=0; i<cnt; i++) {
		sendManchesterByte(transmitterPin, data[i]); 
	}

	digitalWrite(transmitterPin, LOW);  /* Drop the transmitter line */
} 

/**
 * Encrypts, adds checksums and transmits the data. The value of byte 3 in the data is ignored.
 */
void SensorTransmitter::sendPackage(byte transmitterPin, byte *data) {
	byte buffer[14], temp, count;
	for (temp=0x5e; temp>0x40; temp+=0x40) { /* Sends 3 packages */
		memcpy(buffer, data,  ((data[2] >> 1) & 0x1f) + 1);

		buffer[3] = temp;   

		count = encryptAndAddCheck(buffer); /* Encrypt, add checksum bytes */
		sendManchesterPackage(transmitterPin, buffer,count);  /* Send the package */

		delay(30);
	}
}


/************************************
 * Thermo / Hygro sensor transmitter
 ***********************************/
 
ThermoHygroTransmitter::ThermoHygroTransmitter(byte transmitterPin,  byte randomId, byte channel)  : SensorTransmitter(transmitterPin, randomId) {
	_channel = channel;	
}

void ThermoHygroTransmitter::sendTempHumi(int temperature, byte humidity) {
	byte buffer[10];
	
	// Note: temperature is 10x the actual temperature! So, 23.5 degrees is passed as 235.
	
	buffer[0] = 0x75; 		/* Header byte */
	buffer[1] = (_channel << 5) | _randomId ;  /* Thermo-hygro at channel 1 (see table1)*/
	buffer[2] = 0xce;		/* Package size byte for th-sensor */
	
	if ( temperature < 0 ) {
		buffer[5] = 0x4 << 4;		// High nibble is 0x4 for sub zero temperatures...
		temperature = -temperature; // Make temperature positive
	} else {
		buffer[5] = 0xc << 4;		// ...0xc for positive
	}
	
	// Note: temperature is now always positive!
	buffer[4] = (((temperature % 100) / 10 ) << 4) | 	// the "3" from 23.5
				(temperature % 10);						// the "5" from 23.5
	buffer[5] |= (temperature / 100); 					// the "2" from 23.5
	
	buffer[6] = ((humidity / 10) << 4) | (humidity % 10); // BCD encoded
	
	buffer[7]=0xff; 		/* Comfort flag */
	
	sendPackage(_transmitterPin, buffer);
}

