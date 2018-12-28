// Test fastDigital function timing with a scope.
#include <DigitalIO.h>
const uint8_t PIN = 13;
void setup()
{
	fastPinMode(PIN, OUTPUT);
}
void loop()
{
	fastDigitalWrite(PIN, HIGH);
	fastDigitalWrite(PIN, LOW);
	fastDigitalToggle(PIN);
	fastDigitalToggle(PIN);
	delay(1);
}