// Scope test for fast shiftOut function.
#include <DigitalIO.h>

// Create clockPin in output mode with inital level LOW.
DigitalPin<12> clockPin(OUTPUT, LOW);

// Create dataPin in output mode with inital level HIGH.
DigitalPin<13> dataPin(OUTPUT, HIGH);

//------------------------------------------------------------------------------
// Time to send one bit is ten cycles or 625 ns for 16 MHz CPU.
inline __attribute__((always_inline))
void sendBit(uint8_t bit, uint8_t data)
{
	dataPin = data & (1 << bit);
	clockPin = 1;
	// may want a nop here - clock pulse is 125 ns wide
	clockPin = 0;
}
//------------------------------------------------------------------------------
// Time to send one byte is 5 usec.
void shiftOut(uint8_t bits)
{
	sendBit(7, bits);
	sendBit(6, bits);
	sendBit(5, bits);
	sendBit(4, bits);
	sendBit(3, bits);
	sendBit(2, bits);
	sendBit(1, bits);
	sendBit(0, bits);
}
//------------------------------------------------------------------------------
void setup()
{
	// Not used.
}
//------------------------------------------------------------------------------
void loop()
{
	shiftOut(0X55);
	delay(2);
}
