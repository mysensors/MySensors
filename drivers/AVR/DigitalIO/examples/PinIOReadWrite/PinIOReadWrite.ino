// Read pin 12 and write the value to pin 13.
#include <DigitalIO.h>

PinIO readPin(12);
PinIO writePin(13);

void setup()
{
	// Set input mode and disable pull-up.
	readPin.config(INPUT, LOW);

	// set output mode
	writePin.mode(OUTPUT);
}
void loop()
{
	writePin.write(readPin.read());
}