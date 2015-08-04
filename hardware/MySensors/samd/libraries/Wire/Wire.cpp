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

extern "C" {
#include <string.h>
}

#include <Arduino.h>
#include <wiring_private.h>

#include "Wire.h"

TwoWire::TwoWire(SERCOM * s)
{
  this->sercom = s;
  transmissionBegun = false;
}

void TwoWire::begin(void) {
  //Master Mode
  sercom->initMasterWIRE(TWI_CLOCK);
  sercom->enableWIRE();

  pinPeripheral(PIN_WIRE_SDA, g_APinDescription[PIN_WIRE_SDA].ulPinType);
  pinPeripheral(PIN_WIRE_SCL, g_APinDescription[PIN_WIRE_SCL].ulPinType);
}

void TwoWire::begin(uint8_t address) {
  //Slave mode
  sercom->initSlaveWIRE(address);
  sercom->enableWIRE();
}

void TwoWire::setClock(uint32_t frequency) {
	// dummy funtion
}

uint8_t TwoWire::requestFrom(uint8_t address, size_t quantity, bool stopBit)
{
  if(quantity == 0)
  {
    return 0;
  }

  size_t byteRead = 0;

  if(sercom->startTransmissionWIRE(address, WIRE_READ_FLAG))
  {
    // Read first data
    rxBuffer.store_char(sercom->readDataWIRE());

    // Connected to slave
    //while(toRead--)
    for(byteRead = 0; byteRead < quantity; ++byteRead)
    {
      if( byteRead == quantity - 1)  // Stop transmission
      {
        sercom->prepareNackBitWIRE(); // Prepare NACK to stop slave transmission
        //sercom->readDataWIRE(); // Clear data register to send NACK
        sercom->prepareCommandBitsWire(WIRE_MASTER_ACT_STOP); // Send Stop
      }
      else // Continue transmission
      {
        sercom->prepareAckBitWIRE();  // Prepare Acknowledge
        sercom->prepareCommandBitsWire(WIRE_MASTER_ACT_READ); // Prepare the ACK command for the slave
        rxBuffer.store_char( sercom->readDataWIRE() );  // Read data and send the ACK
      }
    }
  }

  return byteRead;
}

uint8_t TwoWire::requestFrom(uint8_t address, size_t quantity)
{
  return requestFrom(address, quantity, true);
}

void TwoWire::beginTransmission(uint8_t address) {
  // save address of target and clear buffer
  txAddress = address;
  txBuffer.clear();

  transmissionBegun = true;
}

// Errors:
//  0 : Success
//  1 : Data too long
//  2 : NACK on transmit of address
//  3 : NACK on transmit of data
//  4 : Other error
uint8_t TwoWire::endTransmission(bool stopBit)
{
  transmissionBegun = false ;

  // Check if there are data to send
  if ( txBuffer.available() == 0)
  {
    return 4 ;
  }

  // Start I2C transmission
  if ( !sercom->startTransmissionWIRE( txAddress, WIRE_WRITE_FLAG ) )
  {
    sercom->prepareCommandBitsWire(WIRE_MASTER_ACT_STOP);
    return 2 ;  // Address error
  }

  // Send all buffer
  while( txBuffer.available() )
  {
    // Trying to send data
    if ( !sercom->sendDataMasterWIRE( txBuffer.read_char() ) )
    {
      sercom->prepareCommandBitsWire(WIRE_MASTER_ACT_STOP);
      return 3 ;  // Nack or error
    }

    if(txBuffer.available() == 0)
    {
      sercom->prepareCommandBitsWire(WIRE_MASTER_ACT_STOP);
    }
  }

  return 0;
}

uint8_t TwoWire::endTransmission()
{
  return endTransmission(true);
}

size_t TwoWire::write(uint8_t ucData)
{
  if(sercom->isMasterWIRE())
  {
    // No writing, without begun transmission or a full buffer
    if ( !transmissionBegun || txBuffer.isFull() )
    {
      return 0 ;
    }

    txBuffer.store_char( ucData ) ;

    return 1 ;
  }
  else
  {
    if(sercom->sendDataSlaveWIRE( ucData ))
    {
      return 1;
    }
  }

  return 0;
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
  //Try to store all data
  for(size_t i = 0; i < quantity; ++i)
  {
    //Return the number of data stored, when the buffer is full (if write return 0)
    if(!write(data[i]))
      return i;
  }

  //All data stored
  return quantity;
}

int TwoWire::available(void)
{
  return rxBuffer.available();
}

int TwoWire::read(void)
{
  return rxBuffer.read_char();
}

int TwoWire::peek(void)
{
  return rxBuffer.peek();
}

void TwoWire::flush(void)
{
  // Do nothing, use endTransmission(..) to force
  // data transfer.
}

void TwoWire::onReceive(void(*function)(int))
{
  onReceiveCallback = function;
}

void TwoWire::onRequest(void(*function)(void))
{
  onRequestCallback = function;
}

void TwoWire::onService(void)
{
  if ( sercom->isSlaveWIRE() )
  {
    //Received data
    if(sercom->isDataReadyWIRE())
    {
      //Store data
      rxBuffer.store_char(sercom->readDataWIRE());

      //Stop or Restart detected
      if(sercom->isStopDetectedWIRE() || sercom->isRestartDetectedWIRE())
      {
        //Calling onReceiveCallback, if exists
        if(onReceiveCallback)
        {
          onReceiveCallback(available());
        }
      }
    }

    //Address Match
    if(sercom->isAddressMatch())
    {
      //Is a request ?
      if(sercom->isMasterReadOperationWIRE())
      {
        //Calling onRequestCallback, if exists
        if(onRequestCallback)
        {
          onRequestCallback();
        }
      }
    }
  }
}

/*
void TwoWire::onService(void)
{
  // Retrieve interrupt status
  uint32_t sr = TWI_GetStatus(twi);

  if (status == SLAVE_IDLE && TWI_STATUS_SVACC(sr)) {
    TWI_DisableIt(twi, TWI_IDR_SVACC);
    TWI_EnableIt(twi, TWI_IER_RXRDY | TWI_IER_GACC | TWI_IER_NACK
        | TWI_IER_EOSACC | TWI_IER_SCL_WS | TWI_IER_TXCOMP);

    srvBufferLength = 0;
    srvBufferIndex = 0;

    // Detect if we should go into RECV or SEND status
    // SVREAD==1 means *master* reading -> SLAVE_SEND
    if (!TWI_STATUS_SVREAD(sr)) {
      status = SLAVE_RECV;
    } else {
      status = SLAVE_SEND;

      // Alert calling program to generate a response ASAP
      if (onRequestCallback)
        onRequestCallback();
      else
        // create a default 1-byte response
        write((uint8_t) 0);
    }
  }

  if (status != SLAVE_IDLE) {
    if (TWI_STATUS_TXCOMP(sr) && TWI_STATUS_EOSACC(sr)) {
      if (status == SLAVE_RECV && onReceiveCallback) {
        // Copy data into rxBuffer
        // (allows to receive another packet while the
        // user program reads actual data)
        for (uint8_t i = 0; i < srvBufferLength; ++i)
          rxBuffer[i] = srvBuffer[i];
        rxBufferIndex = 0;
        rxBufferLength = srvBufferLength;

        // Alert calling program
        onReceiveCallback( rxBufferLength);
      }

      // Transfer completed
      TWI_EnableIt(twi, TWI_SR_SVACC);
      TWI_DisableIt(twi, TWI_IDR_RXRDY | TWI_IDR_GACC | TWI_IDR_NACK
          | TWI_IDR_EOSACC | TWI_IDR_SCL_WS | TWI_IER_TXCOMP);
      status = SLAVE_IDLE;
    }
  }

  if (status == SLAVE_RECV) {
    if (TWI_STATUS_RXRDY(sr)) {
      if (srvBufferLength < BUFFER_LENGTH)
        srvBuffer[srvBufferLength++] = TWI_ReadByte(twi);
    }
  }

  if (status == SLAVE_SEND) {
    if (TWI_STATUS_TXRDY(sr) && !TWI_STATUS_NACK(sr)) {
      uint8_t c = 'x';
      if (srvBufferIndex < srvBufferLength)
        c = srvBuffer[srvBufferIndex++];
      TWI_WriteByte(twi, c);
    }
  }
}
*/

#if WIRE_INTERFACES_COUNT > 0
/*static void Wire_Init(void) {
  pmc_enable_periph_clk(WIRE_INTERFACE_ID);
  PIO_Configure(
      g_APinDescription[PIN_WIRE_SDA].pPort,
      g_APinDescription[PIN_WIRE_SDA].ulPinType,
      g_APinDescription[PIN_WIRE_SDA].ulPin,
      g_APinDescription[PIN_WIRE_SDA].ulPinConfiguration);
  PIO_Configure(
      g_APinDescription[PIN_WIRE_SCL].pPort,
      g_APinDescription[PIN_WIRE_SCL].ulPinType,
      g_APinDescription[PIN_WIRE_SCL].ulPin,
      g_APinDescription[PIN_WIRE_SCL].ulPinConfiguration);

  NVIC_DisableIRQ(WIRE_ISR_ID);
  NVIC_ClearPendingIRQ(WIRE_ISR_ID);
  NVIC_SetPriority(WIRE_ISR_ID, 0);
  NVIC_EnableIRQ(WIRE_ISR_ID);
}*/


TwoWire Wire(&sercom3);

void SERCOM3_Handler(void) {
  Wire.onService();
}

#endif
