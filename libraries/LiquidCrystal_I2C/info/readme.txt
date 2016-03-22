// LiquidCrystal_I2C V2.0 - Mario H. atmega@xs4all.nl
// Mods for Chinese I2C converter board - Murray R. Van Luyn. vanluynm@iinet.net.au

The LiquidCrystal_I2C library is a modified version of the standard LiquidCrystal library as found on 
the Arduino website.
This library is intended to be used when a parallel HD44780 compatible LCD is controlled over I2C using 
a Chinese PCF8574 extender.
4 of the 8 outputs are used for LCD data lines 4 to 7.
4 outputs are used for the Enable, register-select, Read/Write and backlight control lines.

The Chinese PCF8574 extender is available in two versions, the PCF8574 and the PCF8574A.
The only difference between the two is the I2C base address.
The base address for the PCF8574 is 0x27 and the base address for the PCF8574A is 0x4E.
The examples included in this zip file assume the use of an PCF8574 set for address 0x27 
(A0, A1 and A3 un-linked, so pulled high).

For PCF8574 the addressing is:

Jp3	Jp2	Jp1		
A2	A1	A0	Dec	Hex	
L	L	L	32	0x20
L	L	H	33	0x21
L	H	L	34	0x22
L	H	H	35	0x23
H	L	L	36	0x24
H	L	H	37	0x25
H	H	L	38	0x26
H	H	H	39	0x27

For PCF8574A the addressing is:
				
Jp3	Jp2	Jp1		
A2	A1	A0	Dec	Hex	
L	L	L	56	0x38
L	L	H	57	0x39
L	H	L	64	0x40
L	H	H	74	0x4A
H	L	L	75	0x4B
H	L	H	76	0x4C
H	H	L	77	0x4D
H	H	H	78	0x4E

For compatibility reasons this library contains some aliases for functions that are known under different 
names in other libraries. This should make it fairly easy to implement the library in existing sketches 
without changing to much code.
Functions not supported by this library will return nothing at all and in case a return value is expected 
the function will return 0.
