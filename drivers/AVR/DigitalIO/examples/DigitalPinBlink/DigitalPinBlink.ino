#include <DigitalIO.h>
// Create object for pin 13 in output mode and demo toggle().
DigitalPin<13> pin13(OUTPUT);

void setup() {}

void loop()
{
	// toggle is a two byte instruction that executes
	// in two cycles or 125 ns on a 16 MHz CPU
	pin13.toggle();
	delay(250);
}