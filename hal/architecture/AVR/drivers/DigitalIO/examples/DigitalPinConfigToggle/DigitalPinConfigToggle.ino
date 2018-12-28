// Demo of config() and fast toggle() function.
#include <DigitalIO.h>

// Class with compile time pin number.
DigitalPin<13> pin13;

void setup()
{
	// Set mode to OUTPUT and level LOW.
	pin13.config(OUTPUT, LOW);
}
void loop()
{
	// toggle is a two byte instruction that executes
	// in two cycles or 125 ns on a 16 MHz CPU
	pin13.toggle();
	delay(100);
	pin13.toggle();
	delay(400);
}