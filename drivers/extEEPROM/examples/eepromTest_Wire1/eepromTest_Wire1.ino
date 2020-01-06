//Test extEEPROM library.
//Writes the EEPROM full of 32-bit integers and reads them back to verify.
//Wire a button from digital pin 6 to ground, this is used as a start button
//so the sketch doesn't do unnecessary EEPROM writes every time it's reset.
//Jack Christensen 09Jul2014
//Paolo Paolucci 17Mar2016 (fix 28Jun2017)
//Mik 03Jan2017            (configured Library to use the Wire1 as I2C channel)

#include <extEEPROM.h>    //https://github.com/PaoloP74/extEEPROM

//One 24LC256 EEPROMs on the bus
const uint32_t totalKBytes = 32;         //for read and write test functions
extEEPROM eep(kbits_256, 1, 64, 0x57);         //device size, number of devices, page size

const uint8_t btnStart = 6;              //start button

void setup(void)
{
	uint8_t eepStatus;
	pinMode(btnStart, INPUT_PULLUP);
	Serial.begin(115200);
	while (!SerialUSB) {}

	bool channelInsert = false;
	Serial.println(F("Select the number of Wire channel use the eeprom"));
	Serial.println(F("0 = Wire"));
	Serial.println(F("1 = Wire1"));
	Serial.println(F("...."));
	Serial.println(F("x = WIRE_INTERFACES_COUNT"));

	do {
		if (Serial.available()) {
			char I2Cchannel = Serial.read();

			// only number that are less than WIRE_INTERFACES_COUNT are allowed
			if ((I2Cchannel > '0') && (I2Cchannel < ('0' + WIRE_INTERFACES_COUNT))) {
				channelInsert = true;
			}

			switch ((I2Cchannel - '0')) {

			case 0:
				Serial.println(F("Using the default Wire interface"));
				eepStatus = eep.begin(eep.twiClock400kHz);   //go fast!
				break;

			case 1:
				Serial.println(F("Using the Wire1 interface"));
				eepStatus = eep.begin(eep.twiClock400kHz, &Wire1);   //go fast!
				break;

			/*
			   Uncomment till the number of WIRE_INTERFACES_COUNT of your Arduino board
			  case 2:
			  Serial.println(F("Using the Wire2 interface"));
			  eepStatus = eep.begin(eep.twiClock400kHz, &Wire2);   //go fast!
			  break;

			  case 3:
			  Serial.println(F("Using the Wire3 interface"));
			  eepStatus = eep.begin(eep.twiClock400kHz, &Wire3);   //go fast!
			  break;

			  case 4:
			  Serial.println(F("Using the Wire4 interface"));
			  eepStatus = eep.begin(eep.twiClock400kHz, &Wire4);   //go fast!
			  break;

			  case 5:
			  Serial.println(F("Using the Wire5 interface"));
			  eepStatus = eep.begin(eep.twiClock400kHz, &Wire5);   //go fast!
			  break;*/

			default:
				Serial.println(F("A wrong channel has been inserted (Arduino manage max 5)"));
				break;
			}
		}
	} while (!channelInsert);

	if (eepStatus) {
		Serial.print(F("extEEPROM.begin() failed, status = "));
		Serial.println(eepStatus);
		while (1);
	}

	Serial.println(F("Started !!"));

	uint8_t chunkSize =
	    64;    //this can be changed, but must be a multiple of 4 since we're writing 32-bit integers
	//    eeErase(chunkSize, 0, totalKBytes * 1024 - 1);
	eeWrite(chunkSize);
	eeRead(chunkSize);

	dump(0, 32);            //the first 32 bytes
	dump(32256, 64);        //the last 64 bytes
	//dump(32512, 64);        //across the device boundary
	//dump(65520, 16);        //the last 16 bytes
}

void loop(void)
{
}

//write test data (32-bit integers) to eeprom, "chunk" bytes at a time
void eeWrite(uint8_t chunk)
{
	chunk &= 0xFC;                //force chunk to be a multiple of 4
	uint8_t data[chunk];
	uint32_t val = 0;
	Serial.println(F("Writing..."));
	uint32_t msStart = millis();

	for (uint32_t addr = 0; addr < totalKBytes * 1024; addr += chunk) {
		if ( (addr & 0xFFF) == 0 ) {
			Serial.println(addr);
		}
		for (uint8_t c = 0; c < chunk; c += 4) {
			data[c + 0] = val >> 24;
			data[c + 1] = val >> 16;
			data[c + 2] = val >> 8;
			data[c + 3] = val;
			++val;
		}
		eep.write(addr, data, chunk);
	}
	uint32_t msLapse = millis() - msStart;
	Serial.print(F("Write lapse: "));
	Serial.print(msLapse);
	Serial.println(F(" ms"));
}

//read test data (32-bit integers) from eeprom, "chunk" bytes at a time
void eeRead(uint8_t chunk)
{
	chunk &= 0xFC;                //force chunk to be a multiple of 4
	uint8_t data[chunk];
	uint32_t val = 0, testVal;
	Serial.println(F("Reading..."));
	uint32_t msStart = millis();

	for (uint32_t addr = 0; addr < totalKBytes * 1024; addr += chunk) {
		if ( (addr & 0xFFF) == 0 ) {
			Serial.println(addr);
		}
		eep.read(addr, data, chunk);
		for (uint8_t c = 0; c < chunk; c += 4) {
			testVal =  ((uint32_t)data[c + 0] << 24) + ((uint32_t)data[c + 1] << 16) + ((
			               uint32_t)data[c + 2] << 8) + (uint32_t)data[c + 3];
			if (testVal != val) {
				Serial.print(F("Error @ addr "));
				Serial.print(addr + c);
				Serial.print(F(" Expected "));
				Serial.print(val);
				Serial.print(F(" Read "));
				Serial.print(testVal);
				Serial.print(F(" 0x"));
				Serial.println(testVal, HEX);
			}
			++val;
		}
	}
	uint32_t msLapse = millis() - msStart;
	Serial.print(F("Last value: "));
	Serial.print(val);
	Serial.print(F(" Read lapse: "));
	Serial.print(msLapse);
	Serial.println(F(" ms"));
}

//write 0xFF to eeprom, "chunk" bytes at a time
void eeErase(uint8_t chunk, uint32_t startAddr, uint32_t endAddr)
{
	chunk &= 0xFC;                //force chunk to be a multiple of 4
	uint8_t data[chunk];
	Serial.println(F("Erasing..."));
	for (int i = 0; i < chunk; i++) {
		data[i] = 0xFF;
	}
	uint32_t msStart = millis();

	for (uint32_t a = startAddr; a <= endAddr; a += chunk) {
		if ( (a & 0xFFF) == 0 ) {
			Serial.println(a);
		}
		eep.write(a, data, chunk);
	}
	uint32_t msLapse = millis() - msStart;
	Serial.print(F("Erase lapse: "));
	Serial.print(msLapse);
	Serial.print(F(" ms"));
}

//dump eeprom contents, 16 bytes at a time.
//always dumps a multiple of 16 bytes.
void dump(uint32_t startAddr, uint32_t nBytes)
{
	Serial.print(F("EEPROM DUMP 0x"));
	Serial.print(startAddr, HEX);
	Serial.print(F(" 0x"));
	Serial.print(nBytes, HEX);
	Serial.print(F(" "));
	Serial.print(startAddr);
	Serial.print(F(" "));
	Serial.println(nBytes);
	uint32_t nRows = (nBytes + 15) >> 4;

	uint8_t d[16];
	for (uint32_t r = 0; r < nRows; r++) {
		uint32_t a = startAddr + 16 * r;
		eep.read(a, d, 16);
		Serial.print(F("0x"));
		if ( a < 16 * 16 * 16 ) {
			Serial.print(F("0"));
		}
		if ( a < 16 * 16 ) {
			Serial.print(F("0"));
		}
		if ( a < 16 ) {
			Serial.print(F("0"));
		}
		Serial.print(a, HEX);
		Serial.print(F(" "));
		for ( int c = 0; c < 16; c++ ) {
			if ( d[c] < 16 ) {
				Serial.print(F("0"));
				Serial.print(d[c], HEX);
				Serial.print( c == 7 ? "  " : " ");
			}
		}
		Serial.println(F(""));
	}
}
