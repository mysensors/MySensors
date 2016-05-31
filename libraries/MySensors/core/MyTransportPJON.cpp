/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
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
 * Copyright (c) 2013, Majenko Technologies and S.J.Hoeksma * 
 * Copyright (c) 2015, LeoDesigner
 * https://github.com/leodesigner/mysensors-serial-transport
 * Copyright (c) 2016, 4ib3r 
 * https://github.com/4ib3r/Arduino
 * Based on PJON (Padded Jittering Operative Network) Arduino compatible, 
 * multi-master, multi-media communications bus system. 
 * https://github.com/gioblu/PJON
 *
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


#include "MyConfig.h"
#include "MyTransport.h"
#include <stdint.h>


#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include "MyTransport.h"

PJON<SoftwareBitBang> bus;

// We only use SYS_PACK in this application
#define	ICSC_SYS_PACK	0x58

unsigned char _nodeId;
char _data[MY_RS485_MAX_MESSAGE_LENGTH];
uint8_t _packet_len;
unsigned char _packet_from;
bool _packet_received;

unsigned char _recStation;

bool transportSend(uint8_t to, const void* data, uint8_t len)
{
    char *datap = (char*)data;
    unsigned char res = 0;

    // This is how many times to try and transmit before failing.
    unsigned char timeout = 500;
    while(res != ACK) 
    {
        res = bus.send_string(to, datap, len);
        bus.receive();
        delayMicroseconds((rand() % 500) + 100);
        if (timeout-- <= 0) break;
    }
    Serial.print("PJON_SEND: ");
    Serial.print(to, HEX);
    Serial.print(" ");
    Serial.print(len, DEC);
    Serial.print(" -> ");
    for (char i = 0; i < len; i++) {
      if (datap[i] < 15) {
        Serial.print('0');
      }
      Serial.print(datap[i], HEX);
    }
    Serial.print(" res: ");
    Serial.println(res);
    return res == ACK;
}


void _receiver_function(uint8_t _id, uint8_t *payload,uint8_t l) {
  Serial.print("PJON_READ: ");
  Serial.print(_id);
  Serial.print(": ");
  for (char i = 0; i < l; i++) {
    if (payload[i] < 15) {
      Serial.print('0');
    }
    Serial.print(payload[i], HEX);
  }
  Serial.println();
  if (!_packet_received) {
    _packet_len = l;
    _packet_received = true;
    memcpy(_data, (const void *)payload, l); 
  }
};

bool transportInit() {
    bus.set_receiver(_receiver_function);
    bus.set_pin(12);
    bus.begin();
    return true;
}

void transportSetAddress(uint8_t address) {
	bus.set_id(address);
	_nodeId = address;
}

uint8_t transportGetAddress() {
	return _nodeId;
}


bool transportAvailable(uint8_t *to) {
	bus.receive();
	if (_packet_received == true) {
		if (_recStation==BROADCAST_ADDRESS) {
			*to = BROADCAST_ADDRESS;
		}
		else { *to = _nodeId; }
	}
	return _packet_received;
}

uint8_t transportReceive(void* data) {
	bus.receive();
	if (_packet_received) {
		memcpy(data,_data,_packet_len);
		_packet_received = false;
		return _packet_len;
	}
	else {
		return (0);
	}
}

void transportPowerDown() {
	// Nothing to shut down here
}


