/*
 * TWI/I2C library for Arduino Zero
 * Copyright (c) 2015 Arduino LLC. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef TwoWire_h
#define TwoWire_h

#include "Stream.h"
#include "variant.h"

#include "SERCOM.h"
#include "RingBuffer.h"

#define BUFFER_LENGTH 32

class TwoWire : public Stream
{
  public:
    TwoWire(SERCOM *s);
    void begin();
    void begin(uint8_t);
    void setClock(uint32_t); // dummy function

    void beginTransmission(uint8_t);
    uint8_t endTransmission(bool stopBit);
    uint8_t endTransmission(void);

    uint8_t requestFrom(uint8_t address, size_t quantity, bool stopBit);
    uint8_t requestFrom(uint8_t address, size_t quantity);

    size_t write(uint8_t data);
    size_t write(const uint8_t * data, size_t quantity);

    virtual int available(void);
    virtual int read(void);
    virtual int peek(void);
    virtual void flush(void);
    void onReceive(void(*)(int));
    void onRequest(void(*)(void));

    using Print::write;

    void onService(void);

  private:
    SERCOM * sercom;
    bool transmissionBegun;

    // RX Buffer
    RingBuffer rxBuffer;

    //TX buffer
    RingBuffer txBuffer;
    uint8_t txAddress;


    // Service buffer
    //uint8_t srvBuffer[BUFFER_LENGTH];
    //uint8_t srvBufferIndex;
    //uint8_t srvBufferLength;

    // Callback user functions
    void (*onRequestCallback)(void);
    void (*onReceiveCallback)(int);

    // TWI state
    //enum TwoWireStatus
    //{
    //  UNINITIALIZED,
    //  MASTER_IDLE,
    //  MASTER_SEND,
    //  MASTER_RECV,
    //  SLAVE_IDLE,
    //  SLAVE_RECV,
    //  SLAVE_SEND
    //};
    //TwoWireStatus status;

    // TWI clock frequency
    static const uint32_t TWI_CLOCK = 100000;

    // Timeouts
    //static const uint32_t RECV_TIMEOUT = 100000;
    //static const uint32_t XMIT_TIMEOUT = 100000;
};

extern TwoWire Wire;

#endif
