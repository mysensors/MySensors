// Utility sketch to explore DS1307 and demonstrate software I2C Master.
//

#include <DigitalIO.h>

// Set pin numbers for your configuration.
const uint8_t SDA_PIN = A4;
const uint8_t SCL_PIN = A5;

//SoftI2cMaster rtc(SCL_PIN, SDA_PIN);
FastI2cMaster<SCL_PIN, SDA_PIN> rtc;

// i2c 8-bit address for DS1307. low bit is read/write
#define DS1307ADDR 0XD0
//------------------------------------------------------------------------------
/*
 * Read 'count' bytes from the DS1307 starting at 'address'.
 */
uint8_t readDS1307(uint8_t address, uint8_t *buf, uint8_t count)
{

	// Send address of data.
	if (!rtc.transfer(DS1307ADDR | I2C_WRITE, &address, 1)) {
		return false;
	}

	// Read data.
	return rtc.transfer(DS1307ADDR | I2C_READ, buf, count);
}
//------------------------------------------------------------------------------
/*
 * write 'count' bytes to DS1307 starting at 'address'.
 */
uint8_t writeDS1307(uint8_t address, uint8_t *buf, uint8_t count)
{

	// Write address and continue transfer for write of data.
	if (!rtc.transfer(DS1307ADDR | I2C_WRITE, &address, 1, I2C_CONTINUE)) {
		return false;
	}
	// Write data.
	return rtc.transferContinue(buf, count);
}
//------------------------------------------------------------------------------
void setup(void)
{
	Serial.begin(9600);
}
//------------------------------------------------------------------------------
/** Store and print a string in flash memory.*/
#define PgmPrint(x) Serial.print(F(x))
/** Store and print a string in flash memory followed by a CR/LF.*/
#define PgmPrintln(x) Serial.println(F(x))
//-------------------------------------------------------------------------------
// Day of week U.S. convention.
char *Ddd[] = {"Bad", "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
//------------------------------------------------------------------------------
void hexPrint(uint8_t v)
{
	Serial.print(v >> 4, HEX);
	Serial.print(v & 0XF, HEX);
}
//------------------------------------------------------------------------------
void hexPrintln(uint8_t v)
{
	hexPrint(v);
	Serial.println();
}
//------------------------------------------------------------------------------
// read hex input
uint8_t hexRead(uint16_t* v)
{
	bool valid = false;
	uint16_t n = 0;
	while (!Serial.available());
	while (Serial.available()) {
		uint8_t c = Serial.read();
		// Exit if end-of-line.
		if (c == '\n' || c == '\r') {
			delay(10);
			// If CR/LF.
			Serial.read();
			break;
		}
		n <<= 4;
		if ('a' <= c && c <= 'f') {
			n += c - ('a' - 10);
		} else if ('A' <= c && c <= 'F') {
			n += c - ('A' - 10);
		} else if ('0' <= c && c <= '9') {
			n +=  c - '0';
		} else {
			valid = false;
			break;
		}
		delay(10);
		valid = true;
	}
	if (!valid) {
		PgmPrintln("Invalid entry");
		return false;
	}
	*v = n;
	return true;
}
//------------------------------------------------------------------------------
uint8_t bcdRead(uint8_t min, uint8_t max, uint8_t* n)
{
	uint16_t v;
	if (!hexRead(&v)) {
		return false;
	}
	uint8_t d = 10 * (v >> 4) + (v & 0XF);
	if ((v >> 4) > 9 || (v & 0XF) > 9 || d < min || d > max) {
		PgmPrintln("Invalid");
		return false;
	}
	*n = v;
	return true;
}
//------------------------------------------------------------------------------
void displayTime(void)
{
	uint8_t r[8];
	if (!readDS1307(0, r, 8)) {
		PgmPrintln("Read Failed for display time");
		return;
	}
	PgmPrint("The current time is 20");
	// year
	hexPrint(r[6]);
	Serial.write('-');

	// month
	hexPrint(r[5]);
	Serial.write('-');

	// day
	hexPrint(r[4]);
	Serial.write(' ');

	Serial.print(Ddd[r[3] < 8 ? r[3] : 0]);
	Serial.write(' ');

	// hour
	hexPrint(r[2]);
	Serial.write(':');

	// minute
	hexPrint(r[1]);
	Serial.write(':');

	// second
	hexPrintln(r[0]);

	// Control register.
	PgmPrint("Control Register: ");
	hexPrintln(r[7]);
}
//------------------------------------------------------------------------------
// dump registers and 56 bytes of RAM
void dumpAll(void)
{
	uint8_t buf[8];
	for (uint8_t a = 0; a < 64; a += 8) {
		hexPrint(a);
		Serial.write(' ');
		if (!readDS1307(a, buf, 8)) {
			PgmPrint("read failed for dumpAll");
			return;
		}
		for (uint8_t i = 0; i < 8; i++) {
			Serial.write(' ');
			hexPrint(buf[i]);
		}
		Serial.println();
	}
}
//------------------------------------------------------------------------------
void fillNvRam(void)
{
	PgmPrint("Enter HEX value for all NV RAM locations (00-FF): ");
	uint16_t v;
	if (!hexRead(&v)) {
		return;
	}
	hexPrint(v);
	for (uint8_t a = 8; a < 64; a ++) {
		if (!writeDS1307(a, (uint8_t *)&v, 1)) {
			PgmPrintln("write failed for fillNvRam");
		}
	}
}
//------------------------------------------------------------------------------
// set control register
/*
The DS1307 control register is used to control the operation of the SQW/OUT pin.
+-----------------------------------------------+
|BIT 7|BIT 6|BIT 5|BIT 4|BIT 3|BIT 2|BIT 1|BIT 0|
+-----------------------------------------------+
|OUT  |  0  |  0  |SQWE |  0  |  0  | RS1 | RS0 |
+-----------------------------------------------+

OUT (Output control): This bit controls the output level of the SQW/OUT pin
when the square wave output is disabled. If SQWE = 0, the logic level on the
SQW/OUT pin is 1 if OUT = 1 and is 0 if OUT = 0.

SQWE (Square Wave Enable): This bit, when set to a logic 1, will enable the
oscillator output. The frequency of the square wave output depends upon the
value of the RS0 and RS1 bits. With the square wave output set to 1Hz, the
clock registers update on the falling edge of the square wave.

Square wave Output Frequency for SQWE = 1.
RS1 RS0 FREQUENCY
 0   0  1Hz
 0   1  4.096kHz
 1   0  8.192kHz
 1   1  32.768kHz
*/
void setControl(void)
{
	PgmPrintln("SQW/OUT pin: ");
	PgmPrintln("(00) Low");
	PgmPrintln("(10) 1Hz");
	PgmPrintln("(11) 4.096kHz");
	PgmPrintln("(12) 8.192kHz");
	PgmPrintln("(13) 32.768kHz");
	PgmPrintln("(80) High");
	PgmPrint("Enter control: ");
	uint16_t r;
	if (!hexRead(&r)) {
		return;
	}
	hexPrintln(r);
	if (!writeDS1307(7, (uint8_t *)&r, 1)) {
		PgmPrint("Write Failed for setControl");
	}
}
//------------------------------------------------------------------------------
void setDate(void)
{
	uint8_t r[4];
	PgmPrint("Enter year (00-99): ");
	if (!bcdRead(0, 99, &r[3])) {
		return;
	}
	hexPrintln(r[3]);
	PgmPrint("Enter month (01-12): ");
	if (!bcdRead(1, 12, &r[2])) {
		return;
	}
	hexPrintln(r[2]);
	PgmPrint("Enter day (01-31): ");
	if (!bcdRead(1, 31, &r[1])) {
		return;
	}
	hexPrintln(r[1]);
	PgmPrint("Enter day of week (01-07, Sun-Sat): ");
	if (!bcdRead(1, 7, &r[0])) {
		return;
	}
	hexPrintln(r[0]);

	if (!writeDS1307(3, r, 4)) {
		PgmPrintln("Write failed for setDate");
	}
}
//------------------------------------------------------------------------------
void setNvRam()
{
	uint8_t u;
	uint16_t a, v;
	while (true) {
		PgmPrint("Enter zero to quit or Hex address (8-3F):");
		if (!hexRead(&a) || a == 0) {
			return;
		}
		if (a < 8 || a > 0X3F) {
			PgmPrintln("Invalid address");
			return;
		}
		hexPrintln(a);
		if (!readDS1307(a, &u, 1)) {
			PgmPrintln("Read failed");
			return;
		}
		PgmPrint("Current value: ");
		hexPrintln(u);
		PgmPrint("Enter new HEX value (00-FF): ");
		if (!hexRead(&v)) {
			return;
		}
		hexPrintln(v);
		if (!writeDS1307(a, (uint8_t *)&v, 1)) {
			PgmPrint("Write Failed");
			return;
		}
	}
}
//------------------------------------------------------------------------------
void setTime(void)
{
	uint8_t r[3];
	PgmPrint("Enter hours (00-23): ");
	if (!bcdRead(0, 23, &r[2])) {
		return;
	}
	hexPrintln(r[2]);
	PgmPrint("Enter minutes (00-59): ");
	if (!bcdRead(0, 59, &r[1])) {
		return;
	}
	hexPrintln(r[1]);
	PgmPrint("Enter seconds (00-59): ");
	if (!bcdRead(0, 59, &r[0])) {
		return;
	}
	hexPrintln(r[0]);

	if (!writeDS1307(0, r, 3)) {
		PgmPrintln("write failed in setTime");
		return;
	}
}
//------------------------------------------------------------------------------
void loop(void)
{
	Serial.println();
	displayTime();
	while (Serial.read() >= 0) {}
	PgmPrintln("\nOptions are:");
	PgmPrintln("(0) Display date and time");
	PgmPrintln("(1) Set time");
	PgmPrintln("(2) Set date");
	PgmPrintln("(3) Set Control");
	PgmPrintln("(4) Dump all");
	PgmPrintln("(5) Fill NV RAM");
	PgmPrintln("(6) Set NV RAM value");
	PgmPrint("Enter option: ");

	uint16_t n;
	if (!hexRead(&n)) {
		return;
	}
	Serial.println(n, DEC);
	if (n == 0) {
		return;
	}
	Serial.println();
	if (n == 1) {
		setTime();
	} else if (n == 2) {
		setDate();
	} else if (n == 3) {
		setControl();
	} else if (n == 4) {
		dumpAll();
	} else if (n == 5) {
		fillNvRam();
	} else if (n == 6) {
		setNvRam();
	} else {
		PgmPrintln("Invalid option");
	}
}
