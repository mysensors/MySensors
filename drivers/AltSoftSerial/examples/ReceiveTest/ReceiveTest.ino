// AltSoftSerial Receive Test
//
// Transmit data with Serial1 and try to receive
// it with AltSoftSerial.  You must connect a wire
// from Serial1 TX to AltSoftSerial RX.

#include <AltSoftSerial.h>

AltSoftSerial altser;
const int mybaud = 9600;

// Board            Serial1 TX   AltSoftSerial RX
// -----            ----------   ----------------
// Teensy 3.x            1              20
// Teensy 2.0            8 (D3)         10 (C7)
// Teensy++ 2.0          3 (D3)          4 (D4)
// Arduino Leonardo      1              13
// Arduino Mega         18              48

// Serial1 on AVR @ 16 MHz minimum baud is 245
// Serial1 on Teensy 3.2 @ 96 MHz minimum baud is 733

// This example code is in the public domain.

byte sentbyte;
unsigned long prevmillis;
byte testbyte=0xF0;

void setup()
{
	delay(200);
	Serial.begin(9600);
	while (!Serial) ;  // wait for Arduino Serial Monitor
	Serial1.begin(mybaud); // connect a wire from TX1
	altser.begin(mybaud);   // to AltSoftSerial RX
	Serial.println("AltSoftSerial Receive Test");
	prevmillis = millis();
}

void loop()
{
	// transmit a test byte on Serial 1
	if (millis() - prevmillis > 250) {
		sentbyte = testbyte++;
		Serial1.write(sentbyte);
		prevmillis = millis();
	}
	// attempt to receive it by AltSoftSerial
	if (altser.available() > 0) {
		byte b = altser.read();
		Serial.println(b);
		if (b != sentbyte) {
			Serial.println("***** ERROR *****");
		}
	}
}
