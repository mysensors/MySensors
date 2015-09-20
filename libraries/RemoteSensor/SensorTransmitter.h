/*
 * RemoteSensor library v1.0.2 (20130601) for Arduino 1.0
 *
 * This library encodes, encrypts en transmits data to
 * remote weather stations made by Hideki Electronics..
 * 
 * Copyright 2011-2013 by Randy Simons http://randysimons.nl/
 *
 * Parts of this code based on Oopsje's CrestaProtocol.pdf, for which
 * I thank him very much!
 * 
 * License: GPLv3. See license.txt
 */
 
#ifndef SensorTransmitter_h
#define SensorTransmitter_h

#include <Arduino.h>

/**
 * SensorTransmitter provides a generic class to simulate Cresta weather sensors, for use
 * with Cresta weather stations.
 * E.g. http://www.cresta.nl/index.php?Itemid=2&option=com_zoo&view=item&category_id=32&item_id=281&lang=en
 *
 * Cresta is just a brand name. The original OEM seems to be Hideki Electronics. There are
 * other brands which use the same hardware and / or protocol. As far as I know these include
 * Mebus, Irox, Honeywell, Cresta and RST.
 *
 * Hardware required for this library: a 433MHz/434MHz SAW oscillator transmitter, e.g.
 * http://www.sparkfun.com/products/10534
 * http://www.conrad.nl/goto/?product=130428
 */
class SensorTransmitter {
	public:
		/**
		 * Initializes the transmitter. About the random id: "A sensor selects a random value
		 * in the range of column 1 when it is reset. It keeps the same ID until it is reset again."
		 * You can leave it at 0 for most purposes
		 * The transmitter pin is set in OUTPUT mode; you don't have to do this yourself.
		 *
		 * @param transmitterPin Arduino-pin connected to the 433MHz transmitter
		 * @param randomId A "random" value in the range [0..31]
		 */
		SensorTransmitter(byte transmitterPin, byte randomId);
		
		/**
		 * Sends a raw sensor package. Before transmitting, the data is encrypted and checksums are
		 * added. The buffer of the data doesn't need to have room for the checksums, as the data is
		 * copied internally to a new buffer which is always large enough.
		 * However, the data must be valid!
		 *
		 * The data is transmitted 3 times.
		 *
		 * Note that this is a static method.
		 *
		 * @param transmitterPin Arduino-pin connected to the 433MHz transmitter
		 * @param data Pointer to data to transmit
		 */
		static void sendPackage(byte transmitterPin, byte *data);
	
	protected:
		byte _transmitterPin;
		byte _randomId;
		
	private:		
		/**
		 * Sends data as manchester encoded stream
		 */
		static void sendManchesterPackage(byte transmitterPin, byte *data, byte cnt);
		
		/**
		 * Sends a single byte as manchester encoded stream
		 */
		static void sendManchesterByte(byte transmitterPin, byte b);
		
		/**
		 * Encryption and checksum
		 */
		static byte encryptAndAddCheck(byte *buffer);
		static byte secondCheck(byte b);
		static byte encryptByte(byte b);	
};

class ThermoHygroTransmitter : public SensorTransmitter {
	public:
		/**
		 * Mimics a Thermo / Hygro sensor. The channel of this device can be 1..5, but note
		 * that only the more expensive receivers can use channels 4 and 5. However, for use
		 * in conjunction with SensorReceiver this is of no concern.
		 *
		 * @param transmitterPin Arduino-pin connected to the 433MHz transmitter
		 * @param randomId A "random" value in the range [0..31]
		 * @channel The channel of this sensor, range [1..5]
		 * @see SensorTransmitter::SensorTransmitter (constructor)
		 */
		ThermoHygroTransmitter(byte transmitterPin, byte randomId, byte channel);
		
		/**
		 * Sends temperature and humidity.
		 *
		 * @param temperature 10x the actual temperature. You want to send 23,5 degrees, then temperature should be 235.
		 * @param humidty Humidity in percentage-points REL. Thus, for 34% REH humidity should be 34.
		 */
		void sendTempHumi(int temperature, byte humidity);
	
	private:
		byte _channel; // Note: internally, the channels for the thermo/hygro-sensor are mapped as follow:
							// 1=>1, 2=>2, 3=>3, 4=>5, 5=>6.
							// This because interally the rain sensor, UV sensor and anemometer are on channel 4.
};
#endif