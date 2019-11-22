/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * -------------------------------------------------------------------------------
 *
 * Copyright (c) 2013, Majenko Technologies and S.J.Hoeksma
 * Copyright (c) 2015, LeoDesigner
 * https://github.com/leodesigner/mysensors-serial-transport
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of Majenko Technologies.
 ********************************************************************************/

// transport(Serial,0,-1); // serial port, node, dePin (-1 disabled)

// Serial Transport

#ifdef __linux__
#include "SerialPort.h"
#endif

// debug
#if defined(MY_DEBUG_VERBOSE_RS485)
#define RS485_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug
#else
#define RS485_DEBUG(x,...)	//!< debug NULL
#endif


#if defined(MY_RS485_DE_PIN)
#if !defined(MY_RS485_DE_INVERSE)
#define assertDE() hwDigitalWrite(MY_RS485_DE_PIN, HIGH)
#define deassertDE() hwDigitalWrite(MY_RS485_DE_PIN, LOW)
#else
#define assertDE() hwDigitalWrite(MY_RS485_DE_PIN, LOW)
#define deassertDE() hwDigitalWrite(MY_RS485_DE_PIN, HIGH)
#endif
#else
#define assertDE()
#define deassertDE()
#endif

#ifdef MY_RS485_LEGACY_SUPPORT
const uint8_t RS485_HEADER_LENGTH = 6;
// We only use SYS_PACK in this application
#define  ICSC_SYS_PACK 0x58


//storage variables
unsigned char _recCommand;
unsigned char _recStation;
unsigned char _recSender;

#if (MY_IS_GATEWAY == true)
unsigned char _isLegacyNode[8] = { 0 };
#else 
unsigned char _isLegacyNode[8] = { 0xFF };
#endif

#else
const unsigned char RS485_HEADER_LENGTH = 3;
#endif
// Receiving header information
char _header[RS485_HEADER_LENGTH];

// Reception state machine control and storage variables
unsigned char _recPhase;
unsigned char _recPos;

unsigned char _recLen;
unsigned char _recCS;
unsigned char _recCalcCS;

static bool IS_LEGACY_PACK = true;


#if defined(__linux__)
SerialPort _dev = SerialPort(MY_RS485_HWSERIAL);
#elif defined(MY_RS485_HWSERIAL)
HardwareSerial& _dev = MY_RS485_HWSERIAL;
#else
AltSoftSerial _dev;
#endif

unsigned char _nodeId;
unsigned char _hasNodeId = false;
char _data[MY_RS485_MAX_MESSAGE_LENGTH];
uint8_t _packet_len;
unsigned char _packet_from;
bool _packet_received;

// Packet wrapping characters, defined in standard ASCII table
#define SOH 1
#define NEW_SOH 0x11
#define STX 2
#define ETX 3
#define EOT 4



//Reset the state machine and release the data pointer
void _serialReset()
{
	_recPhase = 0;
	_recPos = 0;
	_recLen = 0;
	_recCS = 0;
	_recCalcCS = 0;
	#ifdef MY_RS485_LEGACY_SUPPORT
	_recCommand = 0;
	#endif
}

// flush uart data
void _flush(){
#ifdef __PIC32MX__
	// MPIDE has nothing yet for this.  It uses the hardware buffer, which
	// could be up to 8 levels deep.  For now, let's just delay for 8
	// characters worth.
	delayMicroseconds((F_CPU/MY_RS485_BAUD_RATE)+1);
#else
#if defined(ARDUINO) && ARDUINO >= 100
#if ARDUINO >= 104
	// Arduino 1.0.4 and upwards does it right
	_dev.flush();
#else
	// Between 1.0.0 and 1.0.3 it almost does it - need to compensate
	// for the hardware buffer. Delay for 2 bytes worth of transmission.
	_dev.flush();
	delayMicroseconds((20000000UL/MY_RS485_BAUD_RATE)+1);
#endif
#elif defined(__linux__)
	_dev.flush();
#endif
#endif
}

// This is the main reception state machine.  Progress through the states
// is keyed on either special control characters, or counted number of bytes
// received.  If all the data is in the right format, and the calculated
// checksum matches the received checksum, AND the destination station is
// our station ID, then look for a registered command that matches the
// command code.  If all the above is true, execute the command's
// function.
bool _serialProcess()
{
	#ifdef MY_RS485_LEGACY_SUPPORT
	unsigned char i;
	unsigned isLegacyPackage = 0;
	#endif
	if (!_dev.available()) {
		return false;
	}

	while(_dev.available()) {
		char inch;
		inch = _dev.read();

		switch(_recPhase) {

		// Case 0 looks for the header.  Bytes arrive in the serial interface and get
		// shifted through a header buffer.  When the start and end characters in
		// the buffer match the SOH/STX pair, and the destination station ID matches
		// our ID, save the header information and progress to the next state.
		case 0:
			#ifdef MY_RS485_LEGACY_SUPPORT
			memcpy(&_header[0],&_header[1],RS485_HEADER_LENGTH-1);
			_header[5] = inch;
			if ((_header[0] == SOH) && (_header[5] == STX) && (_header[1] != _header[2])) {
				_recCalcCS = 0;
				_recStation = _header[1];
				_recSender = _header[2];
				_recCommand = _header[3];
				_recLen = _header[4];
				isLegacyPackage = 1;

				for (i=1; i<=4; i++) {
					_recCalcCS += _header[i];
				}
				_recPhase = 1;
				_recPos = 0;

				//Avoid _data[] overflow
				if (_recLen >= MY_RS485_MAX_MESSAGE_LENGTH) {
					_serialReset();
					break;
				}

				//Check if we should process this message
				//We reject the message if we are the sender
				//We reject if we are not the receiver and message is not a broadcast
				if ((_recSender == _nodeId) ||
				        (_recStation != _nodeId &&
				         _recStation != BROADCAST_ADDRESS)) {
					_serialReset();
					break;
				}

				if (_recLen == 0) {
					_recPhase = 2;
				}

			}

			//search for new header
			if ((_header[3] == NEW_SOH) && (_header[5] == STX)) {
				_recLen = _header[4];
			#else
			memcpy(&_header[0],&_header[1],RS485_HEADER_LENGTH-1);
			_header[2] = inch;
			if ((_header[0] == NEW_SOH) && (_header[2] == STX)) {
				_recLen = _header[1];
			#endif  //MY_RS485_LEGACY_SUPPORT
				_recCalcCS = 0;
				_recCalcCS += _recLen;
				_recPhase = 1;
				_recPos = 0;

				//Avoid _data[] overflow
				if (_recLen >= MY_RS485_MAX_MESSAGE_LENGTH) {
					_serialReset();
					break;
				}
				
				if (_recLen == 0) {
					_recPhase = 2;
				}
			}
			break;

		// Case 1 receives the data portion of the packet.  Read in "_recLen" number
		// of bytes and store them in the _data array.
		case 1:
			_data[_recPos++] = inch;
			_recCalcCS += inch;
			if (_recPos == _recLen) {
				_recPhase = 2;
			}
			break;

		// After the data comes a single ETX character.  Do we have it?  If not,
		// reset the state machine to default and start looking for a new header.
		case 2:
			// Packet properly terminated?
			if (inch == ETX) {
				_recPhase = 3;
			} else {
				_serialReset();
			}
			break;

		// Next comes the checksum.  We have already calculated it from the incoming
		// data, so just store the incoming checksum byte for later.
		case 3:
			_recCS = inch;
			_recPhase = 4;
			break;

		// The final state - check the last character is EOT and that the checksum matches.
		// If that test passes, then look for a valid command callback to execute.
		// Execute it if found.
		case 4:
			if (inch == EOT) {
				if (_recCS == _recCalcCS) {
					// First, check for system level commands.  It is possible
					// to register your own callback as well for system level
					// commands which will be called after the system default
					// hook.
					#ifdef MY_RS485_LEGACY_SUPPORT
					if (isLegacyPackage){
						switch (_recCommand) {
						case ICSC_SYS_PACK:
							_packet_from = _data[0];
							_packet_len = _recLen;
							_packet_received = true;
							RS485_DEBUG(PSTR("RS485:RLP:FROM:=%" PRIu8 "\n"),_packet_from );
							// Gateways can switch back to lagecy node to support old bootloaders after reboot
							//#if MY_IS_GATEWAY == true		
							_isLegacyNode[(_packet_from & 0xF0) >> 4] |= _BV(_packet_from & 0x0F); // Mark last node as legacy node
							//#endif
							break;
						}
					}
					else{
					#endif
						_packet_len = _recLen;
						//Check if we should process this message
						//We reject the message if we are the sender
						//Message not surpressed if node ID was not assigned to support auto id
						if ((_data[0] == _nodeId) && (_hasNodeId == true)) {
							_serialReset();
							break;
						}
						_packet_received = true;
					#ifdef MY_RS485_LEGACY_SUPPORT
						_packet_from = _data[0];
						RS485_DEBUG(PSTR("RS485:RNP:FROM:=%" PRIu8 "\n"),_packet_from );
						_isLegacyNode[(_packet_from & 0xF0) >> 4] &= ~_BV(_packet_from & 0x0F); // Mark last node as not legacy node
						break;
					}
					break;
					#endif
				}
			}
			//Clear the data
			_serialReset();
			//Return true, we have processed one command
			return true;
			break;
		}
	}
	return true;
}

bool _transportPackage(const uint8_t to, const void* data, const uint8_t len, const bool isLegacy)
{
	(void)isLegacy;	// unused when no legacy mode
	const char *datap = static_cast<char const *>(data);
	unsigned char i;
	unsigned char cs = 0;

	// This is how many times to try and transmit before failing.
	unsigned char timeout = 10;

	// Let's start out by looking for a collision.  If there has been anything seen in
	// the last millisecond, then wait for a random time and check again.
	while (_serialProcess()) {
		unsigned char del;
		del = rand() % 20;
		for (i = 0; i < del; i++) {
			delay(1);
			_serialProcess();
		}
		timeout--;
		if (timeout == 0) {
			// Failed to transmit!!!
			return false;
		}
	}
	assertDE();
		// Start of header by writing multiple SOH
	#ifdef MY_RS485_LEGACY_SUPPORT
	if (isLegacy){
		RS485_DEBUG(PSTR("RS485:SLP:TO:=%" PRIu8 "\n"),to );
		for(byte w=0; w<MY_RS485_SOH_COUNT; w++) {
			_dev.write(SOH);
		}
		_dev.write(to);  // Destination address
		cs += to;
		_dev.write(_nodeId); // Source address
		cs += _nodeId;
		_dev.write(ICSC_SYS_PACK);  // Command code
		cs += ICSC_SYS_PACK;
	}
	else{
		RS485_DEBUG(PSTR("RS485:SNP:TO:=%" PRIu8 "\n"),to );
		for(byte w=0; w<MY_RS485_SOH_COUNT; w++) {
			_dev.write(NEW_SOH);
		}
	}
	#else
	for(byte w=0; w<MY_RS485_SOH_COUNT; w++) {
		RS485_DEBUG(PSTR("RS485:SNP:TO:=%" PRIu8 "\n"),to );
		_dev.write(NEW_SOH);
	}
	#endif

	_dev.write(len);      // Length of text
	cs += len;
	_dev.write(STX);      // Start of text
	for(i=0; i<len; i++) {
		_dev.write(datap[i]);      // Text bytes
		cs += datap[i];
	}
	_dev.write(ETX);      // End of text
	_dev.write(cs);
	_dev.write(EOT);
	_flush();
	deassertDE();
	//while(_dev.available()){//clear echo
	//	_dev.read();
	//}	
	return true;
}

bool transportSend(const uint8_t to, const void* data, const uint8_t len, const bool noACK)
{
	(void)noACK;	// not implemented

	#ifdef MY_RS485_LEGACY_SUPPORT
	unsigned char isLegacy ;
	#endif

	#ifdef MY_RS485_LEGACY_SUPPORT
	if(to == BROADCAST_ADDRESS){
		 if (!_transportPackage(to, data, len, IS_LEGACY_PACK)){
			 return false;
		 }
		 return _transportPackage(to, data, len, !IS_LEGACY_PACK);	
	}		

	if(to == GATEWAY_ADDRESS){
		return _transportPackage(to, data, len, !IS_LEGACY_PACK);
	}

	isLegacy = (_isLegacyNode[(to & 0xF0) >> 4] & _BV(to & 0x0F));
	RS485_DEBUG(PSTR("_isLegacyNode[0x%x] = 0x%x" PRIu8 "\n"),to & 0xF0 >> 4 , _BV(to & 0x0F));
	return  _transportPackage(to, data, len, isLegacy);
	#else
	return _transportPackage(to, data, len, !IS_LEGACY_PACK);
	#endif
	return true;
}



bool transportInit(void)
{
	// Reset the state machine
	_dev.begin(MY_RS485_BAUD_RATE);
	_serialReset();
	deassertDE();
	return true;
}

void transportSetAddress(const uint8_t address)
{
	_nodeId = address;
	_hasNodeId = true;
}

uint8_t transportGetAddress(void)
{
	return _nodeId;
}


bool transportDataAvailable(void)
{
	_serialProcess();
	return _packet_received;
}

bool transportSanityCheck(void)
{
	// not implemented yet
	return true;
}

uint8_t transportReceive(void* data)
{
	if (_packet_received) {
		memcpy(data,_data,_packet_len);
		_packet_received = false;
		return _packet_len;
	} else {
		return (0);
	}
}

void transportPowerDown(void)
{
	// Nothing to shut down here
}

void transportPowerUp(void)
{
	// not implemented
}

void transportSleep(void)
{
	// not implemented
}

void transportStandBy(void)
{
	// not implemented
}

int16_t transportGetSendingRSSI(void)
{
	// not implemented
	return INVALID_RSSI;
}

int16_t transportGetReceivingRSSI(void)
{
	// not implemented
	return INVALID_RSSI;
}

int16_t transportGetSendingSNR(void)
{
	// not implemented
	return INVALID_SNR;
}

int16_t transportGetReceivingSNR(void)
{
	// not implemented
	return INVALID_SNR;
}

int16_t transportGetTxPowerPercent(void)
{
	// not implemented
	return static_cast<int16_t>(100);
}

int16_t transportGetTxPowerLevel(void)
{
	// not implemented
	return static_cast<int16_t>(100);
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	// not possible
	(void)powerPercent;
	return false;
}
