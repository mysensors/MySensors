// Scope test for write timing of DigitalPin functions.
#include <DigitalIO.h>

// Class with compile time pin number.
DigitalPin<13> pin13;

void setup()
{
	// set mode to OUTPUT
	pin13.mode(OUTPUT);
}
void loop()
{
	pin13.high();
	pin13.low();
	pin13.write(1);
	pin13.write(0);
	pin13.toggle();
	pin13.toggle();
	delay(1);
}