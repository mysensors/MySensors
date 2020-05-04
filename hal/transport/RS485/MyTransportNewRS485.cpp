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

#if defined(__AVR_ATmega328P__)
	#include <crc16.h>		// need to be moved to the right place
#else
uint8_t _crc_ibutton_update(uint8_t crc, uint8_t data)
{
	uint8_t i;
	crc = crc ^ data;
	for (i = 0; i < 8; i++)
	{
		if (crc & 0x01)
			crc = (crc >> 1) ^ 0x8C;
		else
			crc >>= 1;
	}  
   	return crc;
}
#endif

#if defined(ARDUINO_ARCH_ESP32)
#	define _delay_us(x) ets_delay_us(x);
#endif

#define RS485_SEND_MESSAGE_TRY_CNT 10
#define RS485_BUS_AQUISITION_TRY_CNT 50
#define RS485_TRANSMIT_TRY_CNT 50

#define RS485_BIT_DURATION_US (1.f/ MY_RS485_BAUD_RATE) *1000 *1000
#define RS485_BUS_AQUISITION_WAIT_US 5* RS485_BIT_DURATION_US * 10   // ~ 5 Bytes * (Bit period * 10 Bit / transaction (start/stop+8bit)) 

// example
// RS485_BUS_AQUISITION_WAIT_US for BAUD 38400 = 1,3 ms
// 10*50*50 * 1,3 ms = 32s

// TODO Will the watchdog trigger before?


// debug
#if defined(MY_DEBUG_VERBOSE_RS485)
#define RS485_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug
#else
#define RS485_DEBUG(x,...)	//!< debug NULL
#endif


#if defined(MY_RS485_DE_PIN)
#	define setPinModeDE()	hwPinMode(MY_RS485_DE_PIN,OUTPUT)	
#if !defined(MY_RS485_DE_INVERSE)
#	define assertDE() hwDigitalWrite(MY_RS485_DE_PIN, HIGH)
#	define deassertDE() hwDigitalWrite(MY_RS485_DE_PIN, LOW)
#else
#	define assertDE() 		hwDigitalWrite(MY_RS485_DE_PIN, LOW)
#	define deassertDE() 	_dev.flush();\
							hwDigitalWrite(MY_RS485_DE_PIN, HIGH)
#endif
#else
#	define assertDE()
#	define deassertDE()
#	define setPinModeDE()
#endif

#if defined(__linux__)
	SerialPort _dev = SerialPort(MY_RS485_HWSERIAL);
#	ifdef MY_RS485_COLLISION_DETECTION
#		error("MY_RS485_COLLISION_DETECTION only possible on micro controllers.")
#	endif
#elif defined(MY_RS485_HWSERIAL)
	HardwareSerial& _dev = MY_RS485_HWSERIAL;
#else
	AltSoftSerial _dev;
#	ifdef MY_RS485_COLLISION_DETECTION
#		warning("MY_RS485_COLLISION_DETECTION not tested with software serial.")
#	endif
#endif

#ifdef MY_RS485_COLLISION_DETECTION
#	if !defined(MY_RS485_RX_PIN) || !defined(MY_RS485_TX_PIN)
#		error ("For MY_RS485_COLLISION_DETECTION MY_RS485_RX_PIN and MY_RS485_TX_PIN need to be defined.")
#	else
#		define setPinModeRS485()	hwPinMode(MY_RS485_TX_PIN,OUTPUT); \
									hwPinMode(MY_RS485_RX_PIN,INPUT)		
#	endif
#	if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__)
#		define _uart_putc(x) _putchReadback(x)
#		define disableInterrups() noInterrupts()
#		define enableInterrups() interrupts()
#		warning("INFO: Timer 2 will be used for MY_RS485_COLLISION_DETECTION")
#	else
#		error("MY_RS485_COLLISION_DETECTION not implemented for current architecture.")
#	endif
#else
#define _uart_putc(x) _dev.write(x)
#define setPinModeRS485()
#endif


#if (F_CPU / MY_RS485_BAUD_RATE) < 256   // check maybe required prescaling
	static const uint8_t TCNT2_VAL_PER_BIT = (uint8_t)(F_CPU / MY_RS485_BAUD_RATE );
#	define TCNT2_VAL_PER_BIT_DEF
#else
#	if ((F_CPU / MY_RS485_BAUD_RATE / 8) < 256 ) // 8x prescaling required
#		define USE_PRESCALER_8X
		static const uint8_t TCNT2_VAL_PER_BIT = (uint8_t)(F_CPU / MY_RS485_BAUD_RATE / 8);
#		define TCNT2_VAL_PER_BIT_DEF
#	endif
#endif

// double check if all conditions above have been analyzed
#if defined(MY_RS485_COLLISION_DETECTION)
#	if defined(TCNT2_VAL_PER_BIT_DEF)
#		if ((((F_CPU / MY_RS485_BAUD_RATE) * 11)  > (256 * 64)) && defined (ARDUINO_ARCH_AVR))
		// (F_CPU / MY_RS485_BAUD_RATE) * 11) == clock cycles to transmit one uart byte (10 bit) + margin.
		// (256 * 64) Timer0 overflow (256) for arduino core. Prescaler is set to 64 by default. 
#			error "MY_RS485_COLLISION_DETECTION will disable the interrups for too long. Please reduce cpu clock or increase MY_RS485_BAUD_RATE."
#		elif !defined(ARDUINO_ARCH_AVR)
#			error("MY_RS485_COLLISION_DETECTION not implemented for current architecture."))
#		endif
#	else
#		error " TCNT2_VAL_PER_BIT is undefined!"
#	endif
#endif



//storage variables
unsigned char _recCommand;
unsigned char _recStation;
unsigned char _recSender;

const unsigned char RS485_HEADER_LENGTH = 4;
// Receiving header information
char _header[RS485_HEADER_LENGTH];

// Reception state machine control and storage variables
unsigned char _recPhase;
unsigned char _recPos;
unsigned char _recLen;
unsigned char _recCRC;
unsigned char _recCalcCRC;

unsigned char _nodeId;
unsigned char _hasNodeId = false;
char _data[MY_RS485_MAX_MESSAGE_LENGTH];
uint8_t _packet_len;
unsigned char _packet_from;
bool _packet_received;

// Packet wrapping characters, defined in standard ASCII table
#define SOH 0x10
#define STX 0x20

//Reset the state machine and release the data pointer
void _serialReset()
{
	_recPhase = 0;
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
	if (!_dev.available()) {
		return false;
	}

	while(_dev.available()) {
		char inch;
		inch = _dev.read();
		if (_packet_received == true){
			return true;
		}
		switch(_recPhase) {

		// Case 0 looks for the header.  Bytes arrive in the serial interface and get
		// shifted through a header buffer.  When the start and end characters in
		// the buffer match the SOH/STX pair, and the destination station ID matches
		// our ID, save the header information and progress to the next state.
		case 0:
			memcpy(&_header[0],&_header[1],RS485_HEADER_LENGTH-1);
			_header[RS485_HEADER_LENGTH-1] = inch;
			if (((_header[0] & 0xF0) == SOH) && (_header[3] == STX)) {	
				_recCRC = _header[1];
				_recLen = _header[2];
				_recCalcCRC = _crc_ibutton_update(0,_recLen);
				_recPhase = 1;
				_recPos = 0;

				//Avoid _data[] overflow
				if (_recLen >= MY_RS485_MAX_MESSAGE_LENGTH) {
					_serialReset();
					break;
				}
				
				if (_recLen == 0) {
					_serialReset();
					//_recPhase = 2; // what now?
					break;
				}
			}
			break;

		// Case 1 receives the data portion of the packet.  Read in "_recLen" number
		// of bytes and store them in the _data array.
		case 1:
			_data[_recPos++] = inch;
			_recCalcCRC = _crc_ibutton_update(_recCalcCRC,inch);
			if (_recPos == _recLen) {
				_recPhase = 2;
			}
			else{
				break;
			}

		case 2:
			if (_recCRC == _recCalcCRC) {
				//Check if we should process this message
				//We reject the message if we are the sender
				//Message not surpressed if node ID was not assigned to support auto id
				if ((_data[0] == _nodeId) && (_hasNodeId == true)) {
					_serialReset();
					break;
				}
				_packet_len = _recLen;
				_packet_received = true;
				RS485_DEBUG(PSTR("RS485:RNP:FROM:=%" PRIu8 "\n"),_data[0] );
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

#ifdef MY_RS485_COLLISION_DETECTION
uint8_t canTcnt2ValBitStart;  // TCNT2 counter value at the beginning of a bit transaction
typedef enum {
    UART_START_VAL = 0,
    UART_STOP_VAL = 1
} uart_cmd_t;

typedef enum {
    CAN_DOMINANT_LEVEL = 0,
    CAN_RECESSIVE_LEVEL = 1
} can_level_t;

    // Start bit would be too long
    // it will take some clock cycles until the start bit is actually written to the bus 
    // anticipate this delay by subtracting some cnt values
    #ifdef USE_PRESCALER_8X
        static const uint8_t BIT_SETUP_TIME = 50/8;
		static const uint8_t BUS_LOOP_DELAY = (uint8_t)((MY_RS485_BUS_LOOP_DELAY_NS * F_CPU / 1000000000 / 8) + 1);
    #else
        static const uint8_t BIT_SETUP_TIME = 50;
		static const uint8_t BUS_LOOP_DELAY = ((uint8_t)(MY_RS485_BUS_LOOP_DELAY_NS * F_CPU + 1));
    #endif 

	#if (TCNT2_VAL_PER_BIT / 4 < BUS_LOOP_DELAY)
		#error ("MY_RS485_BUS_LOOP_DELAY_NS is to long")
	#elif (TCNT2_VAL_PER_BIT / 6 < BUS_LOOP_DELAY)
		#warning ("MY_RS485_BUS_LOOP_DELAY_NS may be to long")
	#endif

bool _putBitReadback(bool b)
{
// check the current state of the CAN bus
// if a dominant level is set on the CAN bus check if this node caused this in a previous transaction
    uint8_t rxVal;
    rxVal = hwDigitalRead(MY_RS485_RX_PIN);
    uint8_t txVal;
    txVal = hwDigitalRead(MY_RS485_TX_PIN);

	while(!(TIFR2 & _BV(OCF2A)))	//wait for timer overflow

    if (rxVal == CAN_DOMINANT_LEVEL && txVal != CAN_DOMINANT_LEVEL)
        return false; // some other node is sending a dominant bit


    // bus seems idle... write bit value
    if (b)
    {   // 1 is the CAN recessive state
		hwDigitalWrite(MY_RS485_TX_PIN,HIGH);
    }
    else
    {
        // 0 is CAN dominant state
		hwDigitalWrite(MY_RS485_TX_PIN,LOW);
    }

	// NOTE
    // The measured delay between TX Pin and RX Pin echo (through CAN Transceiver) is about 150 ns.
    // One clock cycle at 8 MHz is 150 ns.
    // --> The rx signal should 'immediately' be stable after TX pin has been set
	// Better be on the safe side and insert one NOP
	//while((uint8_t)(TCNT2 - canTcnt2ValBitStart) < BUS_LOOP_DELAY);
    // ensure that the bit is set for 1/MY_RS485_BAUD_RATE time
    //while ((uint8_t)(TCNT2 - canTcnt2ValBitStart) < TCNT2_VAL_PER_BIT)
	_delay_us(1);

	while(!(TIFR2 & _BV(OCF2B)))	//wait for timer overflow
    {
        // check the output while waiting.
        // Do collisions occur, while a logical 1 (CAN recessive bit) is being transmitted?
        // Note: there is actually no need to check, if a logical zero (CAN dominant bit) is driven
        // but it does not hurt... therefore we skip the if-condition and save some bytes for the booloader
        rxVal = hwDigitalRead(MY_RS485_RX_PIN);
        if (b != (bool) rxVal)
            return false;
    }
	TIFR2 = _BV(OCF2A) | _BV(OCF2B);
    // increase start value for next bit to transfer
   // canTcnt2ValBitStart += TCNT2_VAL_PER_BIT;

    return true;
}



bool _putchReadback(uint8_t val)
{

	disableInterrups();
    //canTcnt2ValBitStart = TCNT2 - BIT_SETUP_TIME; 
	TCNT2 = OCF2B;				//reset timer
	TIFR2 = _BV(OCF2A);		//clear all overflow bits
//	while(!(TIFR2 & _BV(OCF2A)));	//wait for timer overflow
//	TIFR2 = _BV(OCF2A);
	// send Start Bit
    if (!_putBitReadback(UART_START_VAL))
    {
		goto _putchReadbackError;
    }


    // send payload
    for (uint8_t i = 0; i < 8; ++i)
    {
        if (!_putBitReadback((val & (1 << i)) >> i))
        {
		goto _putchReadbackError;
        }
    }

    // send Stop Bit
    if (!_putBitReadback(UART_STOP_VAL))
    {
		goto _putchReadbackError;
    }

    //_dev.begin(MY_RS485_BAUD_RATE);   // re-enable USART
	enableInterrups();
    return true;

_putchReadbackError:	// using goto jump for code readability
	enableInterrups();
    return false;
}
#endif  //MY_RS485_COLLISION_DETECTION

// TODO: store stuff into uint16_t to save space?
bool _writeRS485Packet(const void *data, const uint8_t len)
{
    unsigned char crc = _crc_ibutton_update(0,len);
    char *datap = (char *)data;

	// Go for software UART
# 	if defined(MY_RS485_COLLISION_DETECTION)
	hwDigitalWrite(MY_RS485_TX_PIN, CAN_RECESSIVE_LEVEL);
	setPinModeRS485();
	_dev.end();
#	endif

	assertDE();
    for(uint8_t i=0; i<len; i++) {
		crc = _crc_ibutton_update(crc,datap[i]);
	}
    // Start of header by writing SOH
    if(!_uart_putc(SOH))
        goto _writeRS485PacketError;

    if(!_uart_putc(crc)) // checksum
        goto _writeRS485PacketError;

    if(!_uart_putc(len)) // Length of text
        goto _writeRS485PacketError;
    
    if(!_uart_putc(STX)) //Start of text
        goto _writeRS485PacketError;

    for (uint8_t i = 0; i < len; i++)
    {
        if(!_uart_putc(datap[i])) // Text bytes
            goto _writeRS485PacketError;
    }
	deassertDE();

# 	if defined(MY_RS485_COLLISION_DETECTION)
	_dev.begin(MY_RS485_BAUD_RATE);  // re-enable USART
#	endif

    return true;

_writeRS485PacketError:	

# 	if defined(MY_RS485_COLLISION_DETECTION)
	_dev.begin(MY_RS485_BAUD_RATE);  // re-enable USART
#	endif

    return false;
}

bool _transportPackage(const void* data, const uint8_t len)
{
// step 0) repeat for RS485_SEND_MESSAGE_TRY_CNT
// step 1) Listen before talk: Wait RS485_BUS_AQUISITION_TRY_CNT times until bus showed no activity for a certain time (RS485_BUS_AQUISITION_WAIT_US)
// step 2) Try to transmit message for RS485_TRANSMIT_TRY_CNT times

    uint8_t sendMessageCnt = RS485_TRANSMIT_TRY_CNT;

    // step 0 repeat
    while (sendMessageCnt > 0)
    {
        // step 1: wait until bus idle
        uint8_t busAquisitionCnt = RS485_BUS_AQUISITION_TRY_CNT;

        while (busAquisitionCnt > 0)
        {
            if (_serialProcess())
            {
                // bus activity detected ... wait and try again
                _delay_us(RS485_BUS_AQUISITION_WAIT_US);
            }
            else {
                // bus seems idle
                // try to send bitwise for RS485_TRANSMIT_TRY_CNT times

                bool ret = _writeRS485Packet(data, len);
                if ( ret )
                {
                    // message has been successfully sent :)
                    return true;
                }

                // bit transmission failed. Some other node is sending...
                // we need to wait here until some characters . If not all nodes would just try to fire
                _delay_us(RS485_BUS_AQUISITION_WAIT_US);
            }

            --busAquisitionCnt;
        } // while (busAquisitionCnt > 0)

        --sendMessageCnt;
    } // while (sendMessageCnt > 0)

    return false;
}

bool transportSend(const uint8_t to, const void* data, const uint8_t len, const bool noACK)
{
	(void)to;		//not needed
	(void)noACK;	// not implemented
	return _transportPackage(data, len);
	return true;
}



bool transportInit(void)
{
	// Reset the state machine
	_dev.begin(MY_RS485_BAUD_RATE);
	_serialReset();
	deassertDE();
	setPinModeDE();
#ifdef MY_RS485_COLLISION_DETECTION
    // activate timer CNT2 to send bits in equidistant time slices
#	if defined(PRR)
    PRR &= ~_BV(PRTIM2); // ensure that Timer2 is enabled in PRR (Power Reduction Register)
#	else
	PRR1 &= ~_BV(PRTIM2); // ensure that Timer2 is enabled in PRR (Power Reduction Register)
#	endif
	TCCR2A = _BV(WGM21); // set to CTC mode
#	ifdef USE_PRESCALER_8X
    	TCCR2B = _BV(CS21); // set clkTS2 with prescaling factor of /8
#	else
    	TCCR2B = _BV(CS20); // set clkTS2 source to non prescaling
#	endif
	OCR2A = TCNT2_VAL_PER_BIT;
	OCR2B = TCNT2_VAL_PER_BIT - 50 ;

#endif
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
