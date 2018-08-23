// **********************************************************************************
// This sketch is an example of using the SPIFlash library with a Moteino
// that has an onboard SPI Flash chip. This sketch listens to a few serial commands
// Hence type the following commands to interact with the SPI flash memory array:
// - 'd' dumps the first 256bytes of the flash chip to screen
// - 'e' erases the entire memory chip
// - 'i' print manufacturer/device ID
// - [0-9] writes a random byte to addresses [0-9] (either 0xAA or 0xBB)
// Get the SPIFlash library from here: https://github.com/LowPowerLab/SPIFlash
// **********************************************************************************
// Copyright Felix Rusu, LowPowerLab.com
// Library and code by Felix Rusu - felix@lowpowerlab.com
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it
// and/or modify it under the terms of the GNU General
// Public License as published by the Free Software
// Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will
// be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A
// PARTICULAR PURPOSE. See the GNU General Public
// License for more details.
//
// You should have received a copy of the GNU General
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
//
// Licence can be viewed at
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************


#include <SPIFlash.h>    //get it here: https://github.com/LowPowerLab/SPIFlash
#include <SPI.h>

#define SERIAL_BAUD      115200
char input = 0;
long lastPeriod = -1;

#ifdef __AVR_ATmega1284P__
#define LED           15 // Moteino MEGAs have LEDs on D15
#define FLASH_SS      23 // and FLASH SS on D23
#else
#define LED           9 // Moteinos have LEDs on D9
#define FLASH_SS      8 // and FLASH SS on D8
#endif

//////////////////////////////////////////
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0x1F44 for adesto(ex atmel) 4mbit flash
//                             0xEF30 for windbond 4mbit flash
//////////////////////////////////////////
SPIFlash flash(FLASH_SS, 0xEF30);

void setup()
{
	Serial.begin(SERIAL_BAUD);
	Serial.print("Start...");

	if (flash.initialize()) {
		Serial.println("Init OK!");
		Blink(LED, 20, 10);
	} else {
		Serial.println("Init FAIL!");
	}

	delay(1000);
}

void loop()
{
	// Handle serial input (to allow basic DEBUGGING of FLASH chip)
	// ie: display first 256 bytes in FLASH, erase chip, write bytes at first 10 positions, etc
	if (Serial.available() > 0) {
		input = Serial.read();
		if (input == 'd') { //d=dump flash area
			Serial.println("Flash content:");
			int counter = 0;

			while(counter<=256) {
				Serial.print(flash.readByte(counter++), HEX);
				Serial.print('.');
			}

			Serial.println();
		} else if (input == 'e') {
			Serial.print("Erasing Flash chip ... ");
			flash.chipErase();
			while(flash.busy());
			Serial.println("DONE");
		} else if (input == 'i') {
			Serial.print("DeviceID: ");
			Serial.println(flash.readDeviceId(), HEX);
		} else if (input >= 48 && input <= 57) { //0-9
			Serial.print("\nWriteByte(");
			Serial.print(input);
			Serial.print(")");
			flash.writeByte(input-48, (millis()%2) ? 0xaa : 0xbb);
		}
	}

	// Periodically blink the onboard LED while listening for serial commands
	if ((int)(millis()/500) > lastPeriod) {
		lastPeriod++;
		pinMode(LED, OUTPUT);
		digitalWrite(LED, lastPeriod%2);
	}
}

void Blink(byte PIN, int DELAY_MS, byte loops)
{
	pinMode(PIN, OUTPUT);
	while (loops--) {
		digitalWrite(PIN,HIGH);
		delay(DELAY_MS);
		digitalWrite(PIN,LOW);
		delay(DELAY_MS);
	}
}