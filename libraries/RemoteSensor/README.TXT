RemoteSensor library v1.0.1 (20120213) for Arduino 1.0
Made by Randy Simons http://randysimons.nl/

This library provides an easy class for Arduino, to send and receive signals
used by some common weather stations using remote 433MHz sensors, like those
made by Cresta.
E.g. http://www.cresta.nl/index.php?Itemid=2&option=com_zoo&view=item&category_id=32&item_id=281&lang=en

Cresta is just a brandname. The original OEM seems to be Hideki Electronics.
There are other brands which use the same hardware and / or protocol. As far
as I know these include Mebus, Irox, Honeywell, Cresta and RST.

This software is based on the work of Oopsje. See docs/CrestaProtocol.pdf

See SensorTransmitter.h and SensorReciver.h for details!

License: GPLv3. See ./RemoteSensor/license.txt

Latest source and wiki: https://bitbucket.org/fuzzillogic/433mhzforarduino


Installation of library:
 - Make sure Arduino is closed
 - Copy the directory RemoteSensor to the Arduino library directory (usually
   <Sketchbook directory>/libraries/)
   See http://arduino.cc/en/Guide/Libraries for detailed instructions.
 
Default installation sender & demo:
 - Connect tha data-in-pin of a 433MHz transmitter to digital pin 11. See 
   docs/hardware setup.jpg.
   (Note: your hardware may have a different pin configuration!)
 - Start Arduino, and open the example: File -> Examples -> RemoteSensor ->
   ThermoHygroTransmitter
 - Alter addresses/devices to reflect your own setup. Otherwise nothing will
   happen.
 - Compile, upload and run!

Default installation receiver & demo:
 - Connect the data-out-pin of a 433MHz receiver to digital pin 2. See photo.
  (Note: your hardware may have a different pin configuration!)
 - Start Arduino, and open the example: File -> Examples -> RemoteSensor ->
   ThermoHygroReceiver
 - Compile, upload and run
 - Open serial monitor in Arduino (115200 baud)
 - Wait for your 433MHz-sensor to transmit (less than 1 minute), and watch the
   serial monitor


Changelog:
RemoteSensor library v1.0.2 (20130601) for Arduino 1.0
 - Reduced memory usage (Flash, RAM). Because of this, a small backwards
   incompatibility is introduced: The last parameter of
   SensorReceiver::decodeThermoHygro now is a byte instead of short.
 - Dropped support for Arduino pre-1.0

RemoteSensor library v1.0.1 (20120213) for Arduino 0022/1.0
 - Ignore obviously too short or too long signals in SensorReceiver, for better
   recognition rate.
 - Support for Arduino 1.0.

RemoteSensor library v1.0.0 (20110919) for Arduino 0022
 - Initial version