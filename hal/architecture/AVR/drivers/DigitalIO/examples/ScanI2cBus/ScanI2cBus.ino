/* Scan I2C bus for devices.
 *
 * For I2C write addresses: the scanner sends a start,
 * the write address, looks for an ACK, then sends a stop.
 *
 * For I2C read addresses: the scanner sends a start, the
 * read address, looks for an ACK. If there is an ACK, it
 * reads a byte and NACKs it. Finally it sends a stop.
 *
 * When the I2C chip responds to the read address, it
 * outputs data and will miss a stop condition sent
 * immediately after the read address (bus contention).
 *
 * If the I2C chip misses the stop condition, the
 * address scanner will see ghost addresses until the
 * read ends randomly. By reading a byte after any read
 * address that ACKs, there is a chance to NACK the read
 * and properly end the I2C transaction.
 */
#include <DigitalIO.h>

// Set pin numbers for your configuration.
const uint8_t SDA_PIN = A4;
const uint8_t SCL_PIN = A5;

SoftI2cMaster i2c(SCL_PIN, SDA_PIN);
//FastI2cMaster<SCL_PIN, SDA_PIN> i2c;
//------------------------------------------------------------------------------
void setup()
{

	Serial.begin(9600);
	while (!Serial);

	if (!digitalRead(SDA_PIN) && !digitalRead(SCL_PIN)) {
		Serial.println("External pull-up resistors appear to be missing.");
		Serial.println("Many false responses may be detected.");
		Serial.println("Type any character to continue.");

		while (!Serial.available());
		Serial.println();

	}
	uint8_t add = 0;
	bool found = false;
	do {
		bool wr = i2c.transfer(add | I2C_WRITE, 0, 0);
		bool rd = i2c.transfer(add | I2C_READ, 0, 0, I2C_CONTINUE);
		if (rd) {
			uint8_t dummy;
			// Must read byte, send NACK, and issue STOP.
			i2c.transferContinue(&dummy, 1);
		}
		if (rd || wr) {
			found = true;
			Serial.print("Device at address: 0X");
			Serial.print(add, HEX);
			Serial.print(" responds to ");
			if (rd) {
				Serial.print("Read");
			}
			if (rd && wr) {
				Serial.print(" and ");
			}
			if (wr) {
				Serial.print("Write");
			}
			Serial.println('.');
		}
		add += 2;
	} while (add);

	if (!found) {
		Serial.println("No devices found.");
	}
	Serial.println("Done");
}
//------------------------------------------------------------------------------
void loop()
{
	// Not used.
}



