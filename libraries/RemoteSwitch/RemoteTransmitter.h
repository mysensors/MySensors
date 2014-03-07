/*
 * RemoteSwitch library v2.3.0 (20121229) made by Randy Simons http://randysimons.nl/
 *
 * License: GPLv3. See license.txt
 */

#ifndef RemoteTransmitter_h
#define RemoteTransmitter_h

#include <Arduino.h>

/**
* RemoteTransmitter provides a generic class for simulation of common RF remote controls, like the 'Klik aan Klik uit'-system
* (http://www.klikaanklikuit.nl/), used to remotely switch lights etc.
*
* Many of these remotes seem to use a 433MHz SAW resonator and one of these chips: LP801B,  HX2262, PT2262, M3E.
* Datasheet for the HX2262/PT2262 ICs:
* http://www.princeton.com.tw/downloadprocess/downloadfile.asp?mydownload=PT2262_1.pdf
*
* Hardware required for this library: a 433MHz/434MHz SAW oscillator transmitter, e.g.
* http://www.sparkfun.com/products/10534
* http://www.conrad.nl/goto/?product=130428
*
* Notes:
* - Since these chips use (and send!) tri-state inputs (low, high and floating) I use 'trits' instead of 'bits',
*   when appropriate.
* - I measured the period lengths with a scope.  Thus: they work for my remotes, but may fail for yours...
*   A better way would be to calculate the 'target'-timings using the datasheets and the resistor-values on the remotes.
*/
class RemoteTransmitter {
	public:
		/**
		* Constructor.
		*
		* To obtain the correct period length, an oscilloscope is convenient, but you can also read the
		* datasheet of the transmitter, measure the resistor for the oscillator and calculate the frequency.
		*
		* @param pin		output pin on Arduino to which the transmitter is connected
		* @param periodusec	[0..511] Duration of one period, in microseconds. A trit is 6 periods.
		* @param repeats	[0..7] The 2log-Number of times the signal is repeated. The actual number of repeats will be 2^repeats. 2 would be bare minimum, 4 seems robust.
		*/
		RemoteTransmitter(byte pin, unsigned int periodusec, byte repeats);

		/**
		* Encodes the data base on the current object and the given trits. The data can be reused, e.g.
		* for use with the static version of sendTelegram, so you won't need to instantiate costly objects!
		*
		* @return The data suited for use with RemoteTransmitter::sendTelegram.
		*/
		unsigned long encodeTelegram(byte trits[]);

		/**
		* Send a telegram, including synchronization-part.
		*
		* @param trits	Array of size 12. "trits" should be either 0, 1 or 2, where 2 indicates "float"
		*/
		void sendTelegram(byte trits[]);

		/**
		* Send a telegram, including synchronization-part. The data-param encodes the period duration, number of repeats and the actual data.
		* Note: static method, which allows for use in low-mem situations.
		*
		* Format data:
		* pppppppp|prrrdddd|dddddddd|dddddddd (32 bit)
		* p = period (9 bit unsigned integer)
		* r = repeats as 2log. Thus, if r = 3, then signal is sent 2^3=8 times
		* d = data
		*
		* @param data data, period and repeats.
		* @param pin Pin number of the transmitter.
		*/
		static void sendTelegram(unsigned long data, byte pin);

		/**
		* The complement of RemoteReceiver.	Send a telegram with specified code as data.
		*
		* Note: static method, which allows for use in low-mem situations.
		*
		* @param pin		output pin on Arduino to which the transmitter is connected
		* @param periodusec	[0..511] Duration of one period, in microseconds. A trit is 6 periods.
		* @param repeats	[0..7] The 2log-Number of times the signal is repeated. The actual number of repeats will be 2^repeats. 2 would be bare minimum, 4 seems robust.
		* @param code		The code to transmit. Note: only the first 20 bits are used, the rest is ignored. Also see sendTelegram.
		*/
		static void sendCode(byte pin, unsigned long code, unsigned int periodusec, byte repeats);

		/**
		* Compares the data received with RemoteReceive with the data obtained by one of the getTelegram-functions.
		* Period duration and repetitions are ignored by this function; only the data-payload is compared.
		*
		* @return true, if the codes are identical (the 20 least significant bits match)
		*/
		static boolean isSameCode(unsigned long encodedTelegram, unsigned long receivedData);

	protected:
		byte _pin;		// Transmitter output pin
		unsigned int _periodusec;	// Oscillator period in microseconds
		byte _repeats;	// Number over repetitions of one telegram
};

/**
* ActionTransmitter simulates a remote, as sold in the Dutch 'Action' stores. But there are many similar systems on the market.
* If your remote has setting for 5 address bits, and can control 5 devices on or off, then you can try to use the ActionTransmitter.
* Otherwise you may have luck with the ElroTransmitter, which is similar.
*/
class ActionTransmitter: RemoteTransmitter {
	public:
		/**
		* Constructor
		*
		* @param pin		output pin on Arduino to which the transmitter is connected
		* @param periodsec	Duration of one period, in microseconds. Default is 190usec
		* @param repeats	[0..7] The 2log-Number of times the signal is repeated.
		* @see RemoteTransmitter
		*/
		ActionTransmitter(byte pin, unsigned int periodusec=190, byte repeats=4);

		/**
		* Send a on or off signal to a device.
		*
		* @param systemCode	5-bit address (dip switches in remote). Range [0..31]
		* @param device	Device to switch. Range: [A..E] (case sensitive!)
		* @param on	True, to switch on. False to switch off,
		*/
		void sendSignal(byte systemCode, char device, boolean on);

		/**
		* Generates the telegram (data) which can be used for RemoteTransmitter::sendTelegram.
		* See sendSignal for details on the parameters
		*
		* @return Encoded data, including repeats and period duration.
		*/
		unsigned long getTelegram(byte systemCode, char device, boolean on);
};

/**
* BlokkerTransmitter simulates a remote, as sold in the Dutch 'Blokker' stores. But there are many similar systems on the market.
* These remotes have 4 on, 4 off buttons and a switch to switch between device 1-4 and 5-8. No further configuration
* possible.
*/
class BlokkerTransmitter: RemoteTransmitter {
	public:
		/**
		* Constructor
		*
		* @param pin		output pin on Arduino to which the transmitter is connected
		* @param periodsec	Duration of one period, in microseconds. Default is 307usec
		* @param repeats	[0..7] The 2log-Number of times the signal is repeated.
		* @see RemoteTransmitter
		*/
		BlokkerTransmitter(byte pin, unsigned int periodusec=230, byte repeats=4);

		/**
		* Send a on or off signal to a device.
		*
		* @param device	Device to switch. Range: [1..8]
		* @param on	True, to switch on. False to switch off,
		*/
		void sendSignal(byte device, boolean on);

		/**
		* @see RemoteTransmitter::getTelegram
		*/
		unsigned long getTelegram(byte device, boolean on);
};

/**
* KaKuTransmitter simulates a KlikAanKlikUit-remote, but there are many clones.
* If your transmitter has a address dial with the characters A till P, you can try this class.
*/
class KaKuTransmitter: RemoteTransmitter {
	public:
		/**
		* Constructor
		*
		* @param pin		output pin on Arduino to which the transmitter is connected
		* @param periodsec	Duration of one period, in microseconds. Default is 375usec
		* @param repeats	[0..7] The 2log-Number of times the signal is repeated.
		* @see RemoteTransmitter
		*/
		KaKuTransmitter(byte pin, unsigned int periodusec=375, byte repeats=4);

		/**
		* Send a on or off signal to a device.
		*
		* @param address	address (dial switches in remote). Range [A..P] (case sensitive!)
		* @param group	Group to switch. Range: [1..4]
		* @param device	Device to switch. Range: [1..4]
		* @param on	True, to switch on. False to switch off,
		*/
		void sendSignal(char address, byte group, byte device, boolean on);

		/**
		* Send a on or off signal to a device.
		*
		* @param address	address (dip switches in remote). Range [A..P] (case sensitive!)
		* @param device	device (dial switches in remote). Range [1..16]
		* @param on	True, to switch on. False to switch off,
		*/
		void sendSignal(char address, byte device, boolean on);

		/**
		* @see RemoteTransmitter::getTelegram
		*/
		unsigned long getTelegram(char address, byte group, byte device, boolean on);

		/**
		* @see RemoteTransmitter::getTelegram
		*/
		unsigned long getTelegram(char address, byte device, boolean on);
};

/**
* ElroTransmitter simulates remotes of the Elro "Home Control" series
* see http://www.elro.eu/en/m/products/category/home_automation/home_control
* There are are many similar systems on the market. If your remote has setting for 5 address bits, and can control
* 4 devices on or off, then you can try to use the ElroTransmitter. Otherwise you may have luck with the
* ActionTransmitter, which is similar.
*/
class ElroTransmitter: RemoteTransmitter {
	public:
		/**
		* Constructor
		*
		* @param pin		output pin on Arduino to which the transmitter is connected
		* @param periodsec	Duration of one period, in microseconds. Default is 320usec
		* @param repeats	[0..7] The 2log-Number of times the signal is repeated.
		* @see RemoteSwitch
		*/
		ElroTransmitter(byte pin, unsigned int periodusec=320, byte repeats=4);

		/**
		* Send a on or off signal to a device.
		*
		* @param systemCode	5-bit address (dip switches in remote). Range [0..31]
		* @param device	Device to switch. Range: [A..D] (case sensitive!)
		* @param on	True, to switch on. False to switch off,
		*/
		void sendSignal(byte systemCode, char device, boolean on);

		/**
		* Generates the telegram (data) which can be used for RemoteSwitch::sendTelegram.
		* See sendSignal for details on the parameters
		*
		* @return Encoded data, including repeats and period duration.
		*/
		unsigned long getTelegram(byte systemCode, char device, boolean on);
};

#endif
