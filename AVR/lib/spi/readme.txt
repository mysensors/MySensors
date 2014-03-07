This directory contains a simple AVR spi helper library that should
run on most AVR microcontrollers which support SPI.

The code is further described at:
http://www.rocketnumbernine.com/2009/04/26/using-spi-on-an-avr-1/ (basics & ADC)
http://www.rocketnumbernine.com/2009/05/12/using-spi-on-an-avr-2/ (RTC)

Any comments/questions to <andrew@rocketnumbernine.com>

Make sure viewing this text in a fixed width font for the tables and diagrams


----------------------------------
Arduino
----------------------------------
The Arduino Directory contains all code required to run on the Arduino.
Simply copy the Arduino/SPI and Arduino/DS1305 directories to the 
"hardware/libraries" directory of your Arduino IDE installation and
restart the Arduino IDE. Once restarted there should be
"Sketch->Import Library->SPI" and "Sketch->Import Library->DS1305" menu
entries which can be used to include the spi header files into your project.
"File->Sketchbook->Examples->Library SPI" contains 3 example projects


----------------------------------
AVR GCC
----------------------------------
To use, just copy spi.h and spi.c (and ds1305.c and ds1305.h if using
the DS1305 Real Time Clock chip) to your own project and compile.

Makefile.mf - 'standard' generated make file from AVR GCC
Makefile - Makefile "make examples" will make example programs below:

spi-test-adc.c        MC3201 Analogue to Digital convertor example
spi-test-rtc.c        DS1305 - Real-Time-Clock/Alarm example
spi-test-masterslave.c        Master/Slave communication example


----------------------------------
PINS
----------------------------------
Following are the AVR pins used in the circuits below

      ATmegaXXX  Arduino AT90usbXXX
MISO    PB4        12       PB3
MOSI    PB3        11       PB2
SCK     PB5        13       PB1
SS      PB2        10       PB0

INT0    PD2         2       PD0   


----------------------------------
MC3201 12-bit ADC Circuit
----------------------------------
See http://ww1.microchip.com/downloads/en/DeviceDoc/21290D.pdf for full details.

             +-----------------+
             |     MC3201      |
        +5V [| Vref        Vdd |] +5V
             |                 |
Analogue In [| IN+         CLK |] AVR SCK
             |                 |
     Ground [| IN-        Dout |] AVR MOSI
             |                 |
     Ground [| Vss         ^CS |] AVR CE
             +-----------------+


For testing purposes "Analogue In" can be connected to the middle wire
of a potentiometer with one outer wire being connected to +5 and the
other to Ground.  A 1uF Capacitor should be placed between the analogue
input and ground, and +5V and ground as close to the chip as possible,
but in a pinch the circuit will likely work without.


MOSI/SLCK/CE to the relevant pin on the AVR (MISO is not used) - see above.

----------------------------------
DS1305 Real Time Clock Circuit
----------------------------------
See http://datasheets.maxim-ic.com/en/ds/DS1305.pdf for full details.

Without a battery the following connections can be used:

                +-----------------+
                |      DS1305     |
       Ground  [| Vcc2       Vcc1 |] +5V
                |                 |
       Ground  [| Vbat        ^PF |] Not Connected
                |                 |
       Crystal [| X1        Vccif |] +5V
                |                 |
       Crystal [| X2          SDO |] AVR MISO
                |                 |
 Not Connected [| N.C.        SDI |] AVR MOSI
                |                 |
      AVR INT0 [| INT0       SLCK |] AVR SLCK
                |                 |
 Not Connected [| INT1        CE  |] AVR CE
                |                 |
        Ground [| GND     SERMODE |] +5V
                +-----------------+


MISO/MOSI/SLCK/CE/INT0 to the relevant pin on the AVR - see above.





