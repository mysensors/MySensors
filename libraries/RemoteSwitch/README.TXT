RemoteSwitch library v2.3.0 (20121229) for Arduino 1.0
Made by Randy Simons http://randysimons.nl/

This library provides an easy class for Arduino, to send and receive signals
used by some common "old style" 433MHz remote control switches.

There are two styles of remote:
 - "old style", which uses switches or a dial to set the house code. Use this
   library.
 - "new style", which use a button on the receivers to "learn" a signal. Use
   the NewRemoteSwitch library instead.

See RemoteTransmitter.h and RemoteReceiver.h for details!

License: GPLv3. See ./RemoteSwitch/license.txt

Latest source and wiki: https://bitbucket.org/fuzzillogic/433mhzforarduino


Installation of library:
 - Make sure Arduino is closed
 - Copy the directory RemoteSwitch to the Arduino library directory (usually
   <Sketchbook directory>/libraries/)
   See http://arduino.cc/en/Guide/Libraries for detailed instructions.
 
Default installation sender & demo:
 - Connect tha data-in-pin of a 433MHz transmitter to digital pin 11. See 
   docs/hardware setup.jpg.
   (Note: your hardware may have a different pin configuration!)
 - Start Arduino, and open the example: File -> Examples -> RemoteSwitch ->
   LightShow
 - Alter addresses/devices to reflect your own setup. Otherwise nothing will
   happen.
 - Compile, upload and run!

Default installation receiver & demo:
 - Connect the data-out-pin of a 433MHz receiver to digital pin 2. See photo.
   (Note: your hardware may have a different pin configuration!)
 - Start Arduino, and open the example: File -> Examples -> RemoteSwitch ->
   ShowReceivedCode or ShowReceivedCodeNewKaku, depending on your remote.
 - Compile, upload and run
 - Open serial monitor in Arduino (115200 baud)
 - Press buttons on a 433MHz-remote, and watch the serial monitor
 
Notes:
 - These transmitters often use the PT2262 IC, or equivalent. For details about 
   the waveforms used by these remotes, the datasheet of the PT2262 is provided.
   Page 4 and 5 will be of most interest.


Changelog:
RemoteSwitch library v2.3.0 (20121229) for Arduino 1.0
 - Improved reception quality by filtering too short pulses.
 - Dropped pre-v1.0 Arduino support.
 - Added RemoteReceiver::deinit()

RemoteSwitch library v2.2.1 (20120314) for Arduino 0022/1.0
 - Fixed Elro code, it caused memory corruption.
 - Renamed old-style .pde files to new-style .ino

RemoteSwitch library v2.2.0 (20120213) for Arduino 0022/1.0
 - Added support for Elro switches (http://www.elro.eu/en/m/products/category/home_automation/home_control)
   (untested; I don't have these kind of devices)
 - Allowed the number of repeated signals transmitted by RemoteTransmitter to be
   changed.
 - By default, 16 instead of 8 repeats are sent.
 - Support for Arduino 1.0.
 - Added Datasheet PT2262, which (or equivalent) is often used in the transmitters.
 - RemoteReceiver::isReceiving timeout changed to 150ms; 100ms was too short.
 
RemoteSwitch library v2.1.1 (20110920) for Arduino 0022
 - Improved RemoteReceiver::isReceiving
 
RemoteSwitch library v2.1.0 (20110919) for Arduino 0022
 - Changed classnames from *Switch to *Transmitter
 - Added RemoteTransmitter::sendCode. See example "retransmitter".
 - RemoteReceiver::interruptHandler() is now public, for use with InterruptChain

RemoteSwitch library v2.0.0 (20100130) for Arduino 0017
 - Complete rewrite; can now receive and decode signals directly from
   the receiver's digital out.