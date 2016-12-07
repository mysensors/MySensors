// Use begin to assign pin numbers.
// Read pin 12 and write the value to pin 13.
#include <DigitalIO.h>

// Declare the PinIO instances.
PinIO readPin;
PinIO writePin;
//------------------------------------------------------------------------------
void setup()
{
	// Assign pin 12 to readPin.
	readPin.begin(12);

	// input mode with pull-ups disabled
	readPin.config(INPUT, LOW);

	// Assign pin 13 to writePin and set mode to output.
	writePin.begin(13);
	writePin.mode(OUTPUT);
}
//------------------------------------------------------------------------------
void loop()
{
	// Copy the value read from readPin to writePin.
	writePin.write(readPin.read());
}