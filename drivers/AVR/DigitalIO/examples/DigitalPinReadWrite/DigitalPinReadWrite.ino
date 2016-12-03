// Read pin 12 and write value to pin 13.
#include <DigitalIO.h>

DigitalPin<12> pin12(INPUT);
DigitalPin<13> pin13(OUTPUT);

void setup() {}

void loop()
{
	pin13 = pin12;
}