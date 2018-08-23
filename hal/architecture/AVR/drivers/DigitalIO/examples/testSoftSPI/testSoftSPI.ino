// Scope test for development - assumes 328 processor.
#include <DigitalIO.h>

const uint8_t SOFT_SPI_MISO_PIN = 7;
const uint8_t SOFT_SPI_MOSI_PIN = 8;
const uint8_t SOFT_SPI_SCK_PIN  = 9;
const uint8_t SPI_MODE = 0;

SoftSPI<SOFT_SPI_MISO_PIN, SOFT_SPI_MOSI_PIN, SOFT_SPI_SCK_PIN, SPI_MODE> spi;

int test;
void setup()
{
	Serial.begin(9600);
	spi.begin();
	while(1) {
		Serial.println("Enter:");
		Serial.println("R - Receive");
		Serial.println("S - Send");
		Serial.println("T - Transfer");
		while ((test = Serial.read()) <= 0) {}
		test = toupper(test);
		if (strchr("RST", test)) {
			break;
		}
		Serial.println("Invalid entry");
	};
	Serial.print("Starting test ");
	Serial.println((char)test);
}
void loop()
{
	if (test == 'S') {
		spi.send(0X55);
	}
	if (test == 'R') {
		Serial.println(spi.receive(), HEX);
	}
	if (test == 'T') {
		Serial.println(spi.transfer(0XAA), HEX);
	}
	delay(10);
}