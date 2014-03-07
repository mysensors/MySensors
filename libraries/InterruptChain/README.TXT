InterruptChain library v1.3.0 (20130601) for Arduino 1.0
Made by Randy Simons http://randysimons.nl/

This library allows for daisychaining the interrupts, i.e. you can attach
more than one interrupt handler to a single interrupt.

See the examples for usage. See InterruptChainLib.h for details!

License: GPLv3. See ./InterruptChain/license.txt

Latest source and wiki: https://bitbucket.org/fuzzillogic/433mhzforarduino


Installation of library:
 - Make sure Arduino is closed
 - Copy the directory InterruptChain to the Arduino library directory (usually
   <Sketchbook directory>/libraries/)
   See http://arduino.cc/en/Guide/Libraries for detailed instructions.

Examples are provided.


Changelog:
InterruptChain library v1.3.0 (20130601) for Arduino 1.0
 - Dropped support for Arduino pre-1.0
 - Doesn't use recursion internally, and fewer function calls. This should save
   some extra bytes in RAM, at the cost of moderately increased Flash usage.

InterruptChain library v1.2.0 (20120213) for Arduino 0022/1.0
 - Support for Arduino 1.0.

InterruptChain library v1.1.0 (20110921) for Arduino 0022
 - Added enable, disable. Expanded setMode.

InterruptChain library v1.0.0 (20110919) for Arduino 0022
 - First release