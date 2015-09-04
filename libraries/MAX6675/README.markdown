MAX6675 Arduino Library
=======================
Version: **2.0.1**

This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-sa/3.0/">Creative Commons Attribution-ShareAlike 3.0 Unported License</a>.


Summary
-------

The primary use of the library is to easily interface with a MAX6675 chip via it's SPI interface.  Use the following code to initialize the library.  V2 also greatly improves performance and timing of the chip for more accurate reads.  Thanks Ed!

	MAX6675 temp0(CS,SO,SCK,units);

Variables:	

*	CS is chip select pin of chip
*	SO is data out of the chip
*	SCK is the clock pin of the chip
*	units is one of the following:
	*	0 = raw 0-4095 value
	*	1 = temp in ˚C
	*	2 = temp in ˚F
	
Following this you can use the _read_temp()_ function to return the temperature as a float.

	temperature = temp0.read_temp();
	
