/*
  MAX6675.cpp - Library for reading temperature from a MAX6675.

  This work is licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License.
  http://creativecommons.org/licenses/by-sa/3.0/
*/

#include <MAX6675.h>

MAX6675::MAX6675(uint8_t CS_pin, uint8_t SO_pin, uint8_t SCK_pin, uint8_t units)
{
    pinMode(CS_pin, OUTPUT);
    pinMode(SO_pin, INPUT);
    pinMode(SCK_pin, OUTPUT);

    digitalWrite(CS_pin, HIGH);

    _CS_pin = CS_pin;
    _SO_pin = SO_pin;
    _SCK_pin = SCK_pin;
    _units = units;
}

float MAX6675::read_temp()
{
    uint16_t value = 0;
    uint8_t error_tc = 0;
    float temp = 0.0;

    /*
      Initiate a temperature conversion. According to MAX's tech notes FAQ's
      for the chip, Line going high initiates a conversion, which means, we
      need to clock the chip low to high to initiate the conversion, then wait
      for the conversion to be complete before trying to read the data from
      the chip.
    */
    digitalWrite(_CS_pin,LOW);
    delay(2);
    digitalWrite(_CS_pin,HIGH);
    delay(220);

    /* Read the chip and return the raw temperature value */

    /*
      Bring CS pin low to allow us to read the data from
      the conversion process
    */
    digitalWrite(_CS_pin,LOW);

    /* Cycle the clock for dummy bit 15 */
    digitalWrite(_SCK_pin,HIGH);
    delay(1);
    digitalWrite(_SCK_pin,LOW);

     /*
      Read bits 14-3 from MAX6675 for the Temp. Loop for each bit reading
      the value and storing the final value in 'temp'
    */
    for (int i=11; i>=0; i--) {
        digitalWrite(_SCK_pin,HIGH);
        value += digitalRead(_SO_pin) << i;
        digitalWrite(_SCK_pin,LOW);
    }

    /* Read the TC Input inp to check for TC Errors */
    digitalWrite(_SCK_pin,HIGH);
    error_tc = digitalRead(_SO_pin);
    digitalWrite(_SCK_pin,LOW);

    /*
      Read the last two bits from the chip, faliure to do so will result
      in erratic readings from the chip.
    */
    for (int i=1; i>=0; i--) {
        digitalWrite(_SCK_pin,HIGH);
        delay(1);
        digitalWrite(_SCK_pin,LOW);
    }

    // Disable Device
    digitalWrite(_CS_pin, HIGH);

    /*
      Keep in mind that the temp that was just read is on the digital scale
      from 0˚C to 1023.75˚C at a resolution of 2^12.  We now need to convert
      to an actual readable temperature (this drove me nuts until I figured
      this out!).  Now multiply by 0.25.  I tried to avoid float math but
      it is tough to do a good conversion to ˚F.  THe final value is converted
      to an int and returned at x10 power.

      2 = temp in deg F
      1 = temp in deg C
      0 = raw chip value 0-4095
    */
    if(_units == 2) {
        temp = (value*0.25) * 9.0/5.0 + 32.0;
    } else if(_units == 1) {
        temp = (value*0.25);
    } else {
        temp = value;
    }

    /* Output negative of CS_pin if there is a TC error, otherwise return 'temp' */
    if(error_tc != 0) {
        return -_CS_pin;
    } else {
        return temp;
    }
}
