//Test extEEPROM library.
//Writes the EEPROM full of 32-bit integers and reads them back to verify.
//Wire a button from digital pin 6 to ground, this is used as a start button
//so the sketch doesn't do unnecessary EEPROM writes every time it's reset.
//Jack Christensen 09Jul2014
//Paolo Paolucci 17Mar2016 (fix 28Jun2017)

#include <extEEPROM.h>    //https://github.com/PaoloP74/extEEPROM

//One 24LC256 EEPROMs on the bus
const uint32_t totalKBytes = 32;         //for read and write test functions
extEEPROM eep(kbits_256, 1, 64);         //device size, number of devices, page size

const uint8_t btnStart = 6;              //start button

void setup(void)
{
	pinMode(btnStart, INPUT_PULLUP);
	Serial.begin(115200);
	uint8_t eepStatus = eep.begin(eep.twiClock400kHz);   //go fast!
	if (eepStatus) {
		Serial.print(F("extEEPROM.begin() failed, status = "));
		Serial.println(eepStatus);
		while (1);
	}

	Serial.println(F("Press button to start..."));
	while (digitalRead(btnStart) == HIGH) {
		delay(10);    //wait for button push
	}

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
