// Scope test for write timing with Arduino digitaWrite().
const uint8_t PIN13 = 13;

void setup()
{
	// Set mode to OUTPUT.
	pinMode(PIN13, OUTPUT);
}
void loop()
{
	digitalWrite(PIN13, HIGH);
	digitalWrite(PIN13, LOW);
	digitalWrite(PIN13, HIGH);
	digitalWrite(PIN13, LOW);
	delay(1);
}