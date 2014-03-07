// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

#include "DallasTemperature.h"

extern "C" {
  #include "Arduino.h"
}

DallasTemperature::DallasTemperature(OneWire* _oneWire)
  #if REQUIRESALARMS
  : _AlarmHandler(&defaultAlarmHandler)
  #endif
{
  _wire = _oneWire;
  devices = 0;
  parasite = false;
  conversionDelay = TEMP_9_BIT;
}

// initialize the bus
void DallasTemperature::begin(void)
{
  DeviceAddress deviceAddress;

  _wire->reset_search();
  devices = 0; // Reset the number of devices when we enumerate wire devices

  while (_wire->search(deviceAddress))
  {
    if (validAddress(deviceAddress))
    {
      if (!parasite && readPowerSupply(deviceAddress)) parasite = true;

      ScratchPad scratchPad;

      readScratchPad(deviceAddress, scratchPad);

      if (deviceAddress[0] == DS18S20MODEL) conversionDelay = TEMP_12_BIT; // 750 ms
      else if (scratchPad[CONFIGURATION] > conversionDelay) conversionDelay = scratchPad[CONFIGURATION];

      devices++;
    }
  }
}

// returns the number of devices found on the bus
uint8_t DallasTemperature::getDeviceCount(void)
{
  return devices;
}

// returns true if address is valid
bool DallasTemperature::validAddress(uint8_t* deviceAddress)
{
  return (_wire->crc8(deviceAddress, 7) == deviceAddress[7]);
}

// finds an address at a given index on the bus
// returns true if the device was found
bool DallasTemperature::getAddress(uint8_t* deviceAddress, uint8_t index)
{
  uint8_t depth = 0;

  _wire->reset_search();

  while (depth <= index && _wire->search(deviceAddress))
  {
    if (depth == index && validAddress(deviceAddress)) return true;
    depth++;
  }

  return false;
}

// attempt to determine if the device at the given address is connected to the bus
bool DallasTemperature::isConnected(uint8_t* deviceAddress)
{
  ScratchPad scratchPad;
  return isConnected(deviceAddress, scratchPad);
}

// attempt to determine if the device at the given address is connected to the bus
// also allows for updating the read scratchpad
bool DallasTemperature::isConnected(uint8_t* deviceAddress, uint8_t* scratchPad)
{
  readScratchPad(deviceAddress, scratchPad);
  return (_wire->crc8(scratchPad, 8) == scratchPad[SCRATCHPAD_CRC]);
}

// read device's scratch pad
void DallasTemperature::readScratchPad(uint8_t* deviceAddress, uint8_t* scratchPad)
{
  // send the command
  _wire->reset();
  _wire->select(deviceAddress);
  _wire->write(READSCRATCH);

  // read the response

  // byte 0: temperature LSB
  scratchPad[TEMP_LSB] = _wire->read();

  // byte 1: temperature MSB
  scratchPad[TEMP_MSB] = _wire->read();

  // byte 2: high alarm temp
  scratchPad[HIGH_ALARM_TEMP] = _wire->read();

  // byte 3: low alarm temp
  scratchPad[LOW_ALARM_TEMP] = _wire->read();

  // byte 4:
  // DS18S20: store for crc
  // DS18B20 & DS1822: configuration register
  scratchPad[CONFIGURATION] = _wire->read();

  // byte 5:
  // internal use & crc
  scratchPad[INTERNAL_BYTE] = _wire->read();

  // byte 6:
  // DS18S20: COUNT_REMAIN
  // DS18B20 & DS1822: store for crc
  scratchPad[COUNT_REMAIN] = _wire->read();

  // byte 7:
  // DS18S20: COUNT_PER_C
  // DS18B20 & DS1822: store for crc
  scratchPad[COUNT_PER_C] = _wire->read();

  // byte 8:
  // SCTRACHPAD_CRC
  scratchPad[SCRATCHPAD_CRC] = _wire->read();

  _wire->reset();
}

// writes device's scratch pad
void DallasTemperature::writeScratchPad(uint8_t* deviceAddress, const uint8_t* scratchPad)
{
  _wire->reset();
  _wire->select(deviceAddress);
  _wire->write(WRITESCRATCH);
  _wire->write(scratchPad[HIGH_ALARM_TEMP]); // high alarm temp
  _wire->write(scratchPad[LOW_ALARM_TEMP]); // low alarm temp
  // DS18S20 does not use the configuration register
  if (deviceAddress[0] != DS18S20MODEL) _wire->write(scratchPad[CONFIGURATION]); // configuration
  _wire->reset();
  // save the newly written values to eeprom
  _wire->write(COPYSCRATCH, parasite);
  if (parasite) delay(10); // 10ms delay
  _wire->reset();
}

// reads the device's power requirements
bool DallasTemperature::readPowerSupply(uint8_t* deviceAddress)
{
  bool ret = false;
  _wire->reset();
  _wire->select(deviceAddress);
  _wire->write(READPOWERSUPPLY);
  if (_wire->read_bit() == 0) ret = true;
  _wire->reset();
  return ret;
}

// returns the current resolution, 9-12
uint8_t DallasTemperature::getResolution(uint8_t* deviceAddress)
{
  if (deviceAddress[0] == DS18S20MODEL) return 9; // this model has a fixed resolution

  ScratchPad scratchPad;
  readScratchPad(deviceAddress, scratchPad);
  switch (scratchPad[CONFIGURATION])
  {
    case TEMP_12_BIT:
      return 12;
      break;
    case TEMP_11_BIT:
      return 11;
      break;
    case TEMP_10_BIT:
      return 10;
      break;
    case TEMP_9_BIT:
      return 9;
      break;
  }
}

// set resolution of a device to 9, 10, 11, or 12 bits
void DallasTemperature::setResolution(uint8_t* deviceAddress, uint8_t newResolution)
{
  ScratchPad scratchPad;
  if (isConnected(deviceAddress, scratchPad))
  {
    // DS18S20 has a fixed 9-bit resolution
    if (deviceAddress[0] != DS18S20MODEL)
    {
      switch (newResolution)
      {
        case 12:
          scratchPad[CONFIGURATION] = TEMP_12_BIT;
          break;
        case 11:
          scratchPad[CONFIGURATION] = TEMP_11_BIT;
          break;
        case 10:
          scratchPad[CONFIGURATION] = TEMP_10_BIT;
          break;
        case 9:
        default:
          scratchPad[CONFIGURATION] = TEMP_9_BIT;
          break;
      }
      writeScratchPad(deviceAddress, scratchPad);
    }
  }
}

// sends command for all devices on the bus to perform a temperature
void DallasTemperature::requestTemperatures(void)
{
  _wire->reset();
  _wire->skip();
  _wire->write(STARTCONVO, parasite);

  switch (conversionDelay)
  {
    case TEMP_9_BIT:
      delay(94);
      break;
    case TEMP_10_BIT:
      delay(188);
      break;
    case TEMP_11_BIT:
      delay(375);
      break;
    case TEMP_12_BIT:
    default:
      delay(750);
      break;
  }
}

// sends command for one device to perform a temperature by address
void DallasTemperature::requestTemperaturesByAddress(uint8_t* deviceAddress)
{
  _wire->reset();
  _wire->select(deviceAddress);
  _wire->write(STARTCONVO, parasite);

  switch (conversionDelay)
  {
    case TEMP_9_BIT:
      delay(94);
      break;
    case TEMP_10_BIT:
      delay(188);
      break;
    case TEMP_11_BIT:
      delay(375);
      break;
    case TEMP_12_BIT:
    default:
      delay(750);
      break;
  }
}

// sends command for one device to perform a temp conversion by index
void DallasTemperature::requestTemperaturesByIndex(uint8_t deviceIndex)
{
  DeviceAddress deviceAddress;
  getAddress(deviceAddress, deviceIndex);
  requestTemperaturesByAddress(deviceAddress);
}


// Fetch temperature for device index
float DallasTemperature::getTempCByIndex(uint8_t deviceIndex)
{
  DeviceAddress deviceAddress;
  getAddress(deviceAddress, deviceIndex);
  return getTempC((uint8_t*)deviceAddress);
}

// Fetch temperature for device index
float DallasTemperature::getTempFByIndex(uint8_t deviceIndex)
{
  return DallasTemperature::toFahrenheit(getTempCByIndex(deviceIndex));
}

// reads scratchpad and returns the temperature in degrees C
float DallasTemperature::calculateTemperature(uint8_t* deviceAddress, uint8_t* scratchPad)
{
  int16_t rawTemperature = (((int16_t)scratchPad[TEMP_MSB]) << 8) | scratchPad[TEMP_LSB];

  switch (deviceAddress[0])
  {
    case DS18B20MODEL:
    case DS1822MODEL:
      switch (scratchPad[CONFIGURATION])
      {
        case TEMP_12_BIT:
          return (float)rawTemperature * 0.0625;
          break;
        case TEMP_11_BIT:
          return (float)(rawTemperature >> 1) * 0.125;
          break;
        case TEMP_10_BIT:
          return (float)(rawTemperature >> 2) * 0.25;
          break;
        case TEMP_9_BIT:
          return (float)(rawTemperature >> 3) * 0.5;
          break;
      }
      break;
    case DS18S20MODEL:
      /*

      Resolutions greater than 9 bits can be calculated using the data from
      the temperature, COUNT REMAIN and COUNT PER °C registers in the
      scratchpad. Note that the COUNT PER °C register is hard-wired to 16
      (10h). After reading the scratchpad, the TEMP_READ value is obtained
      by truncating the 0.5°C bit (bit 0) from the temperature data. The
      extended resolution temperature can then be calculated using the
      following equation:

                                       COUNT_PER_C - COUNT_REMAIN
      TEMPERATURE = TEMP_READ - 0.25 + --------------------------
                                               COUNT_PER_C
      */

      // Good spot. Thanks Nic Johns for your contribution
      return (float)(rawTemperature >> 1) - 0.25 +((float)(scratchPad[COUNT_PER_C] - scratchPad[COUNT_REMAIN]) / (float)scratchPad[COUNT_PER_C] );
      break;
  }
}

// returns temperature in degrees C or DEVICE_DISCONNECTED if the
// device's scratch pad cannot be read successfully.
// the numeric value of DEVICE_DISCONNECTED is defined in
// DallasTemperature.h.  it is a large negative number outside the
// operating range of the device
float DallasTemperature::getTempC(uint8_t* deviceAddress)
{
  // TODO: Multiple devices (up to 64) on the same bus may take some time to negotiate a response
  // What happens in case of collision?

  ScratchPad scratchPad;
  if (isConnected(deviceAddress, scratchPad)) return calculateTemperature(deviceAddress, scratchPad);
  return DEVICE_DISCONNECTED;
}

// returns temperature in degrees F
float DallasTemperature::getTempF(uint8_t* deviceAddress)
{
  return toFahrenheit(getTempC(deviceAddress));
}

// returns true if the bus requires parasite power
bool DallasTemperature::isParasitePowerMode(void)
{
  return parasite;
}

#if REQUIRESALARMS

/*

ALARMS:

TH and TL Register Format

BIT 7 BIT 6 BIT 5 BIT 4 BIT 3 BIT 2 BIT 1 BIT 0
  S    2^6   2^5   2^4   2^3   2^2   2^1   2^0

Only bits 11 through 4 of the temperature register are used
in the TH and TL comparison since TH and TL are 8-bit
registers. If the measured temperature is lower than or equal
to TL or higher than or equal to TH, an alarm condition exists
and an alarm flag is set inside the DS18B20. This flag is
updated after every temperature measurement; therefore, if the
alarm condition goes away, the flag will be turned off after
the next temperature conversion.

*/

// sets the high alarm temperature for a device in degrees celsius
// accepts a float, but the alarm resolution will ignore anything
// after a decimal point.  valid range is -55C - 125C
void DallasTemperature::setHighAlarmTemp(uint8_t* deviceAddress, char celsius)
{
  // make sure the alarm temperature is within the device's range
  if (celsius > 125) celsius = 125;
  else if (celsius < -55) celsius = -55;

  ScratchPad scratchPad;
  if (isConnected(deviceAddress, scratchPad))
  {
    scratchPad[HIGH_ALARM_TEMP] = (uint8_t)celsius;
    writeScratchPad(deviceAddress, scratchPad);
  }
}

// sets the low alarm temperature for a device in degreed celsius
// accepts a float, but the alarm resolution will ignore anything
// after a decimal point.  valid range is -55C - 125C
void DallasTemperature::setLowAlarmTemp(uint8_t* deviceAddress, char celsius)
{
  // make sure the alarm temperature is within the device's range
  if (celsius > 125) celsius = 125;
  else if (celsius < -55) celsius = -55;

  ScratchPad scratchPad;
  if (isConnected(deviceAddress, scratchPad))
  {
    scratchPad[LOW_ALARM_TEMP] = (uint8_t)celsius;
    writeScratchPad(deviceAddress, scratchPad);
  }
}

// returns a char with the current high alarm temperature or
// DEVICE_DISCONNECTED for an address
char DallasTemperature::getHighAlarmTemp(uint8_t* deviceAddress)
{
  ScratchPad scratchPad;
  if (isConnected(deviceAddress, scratchPad)) return (char)scratchPad[HIGH_ALARM_TEMP];
  return DEVICE_DISCONNECTED;
}

// returns a char with the current low alarm temperature or
// DEVICE_DISCONNECTED for an address
char DallasTemperature::getLowAlarmTemp(uint8_t* deviceAddress)
{
  ScratchPad scratchPad;
  if (isConnected(deviceAddress, scratchPad)) return (char)scratchPad[LOW_ALARM_TEMP];
  return DEVICE_DISCONNECTED;
}

// resets internal variables used for the alarm search
void DallasTemperature::resetAlarmSearch()
{
  alarmSearchJunction = -1;
  alarmSearchExhausted = 0;
  for(uint8_t i = 0; i < 7; i++)
    alarmSearchAddress[i] = 0;
}

// This is a modified version of the OneWire::search method.
//
// Also added the OneWire search fix documented here:
// http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295
//
// Perform an alarm search. If this function returns a '1' then it has
// enumerated the next device and you may retrieve the ROM from the
// OneWire::address variable. If there are no devices, no further
// devices, or something horrible happens in the middle of the
// enumeration then a 0 is returned.  If a new device is found then
// its address is copied to newAddr.  Use
// DallasTemperature::resetAlarmSearch() to start over.
bool DallasTemperature::alarmSearch(uint8_t* newAddr)
{
  uint8_t i;
  char lastJunction = -1;
  uint8_t done = 1;

  if (alarmSearchExhausted) return false;
  if (!_wire->reset()) return false;

  // send the alarm search command
  _wire->write(0xEC, 0);

  for(i = 0; i < 64; i++)
  {
    uint8_t a = _wire->read_bit( );
    uint8_t nota = _wire->read_bit( );
    uint8_t ibyte = i / 8;
    uint8_t ibit = 1 << (i & 7);

    // I don't think this should happen, this means nothing responded, but maybe if
    // something vanishes during the search it will come up.
    if (a && nota) return false;

    if (!a && !nota)
    {
      if (i == alarmSearchJunction)
      {
        // this is our time to decide differently, we went zero last time, go one.
        a = 1;
        alarmSearchJunction = lastJunction;
      }
      else if (i < alarmSearchJunction)
      {
        // take whatever we took last time, look in address
        if (alarmSearchAddress[ibyte] & ibit) a = 1;
        else
        {
          // Only 0s count as pending junctions, we've already exhasuted the 0 side of 1s
          a = 0;
          done = 0;
          lastJunction = i;
        }
      }
      else
      {
        // we are blazing new tree, take the 0
        a = 0;
        alarmSearchJunction = i;
        done = 0;
      }
      // OneWire search fix
      // See: http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1238032295
    }

    if (a) alarmSearchAddress[ibyte] |= ibit;
    else alarmSearchAddress[ibyte] &= ~ibit;

    _wire->write_bit(a);
  }

  if (done) alarmSearchExhausted = 1;
  for (i = 0; i < 8; i++) newAddr[i] = alarmSearchAddress[i];
  return true;
}

// returns true if device address has an alarm condition
bool DallasTemperature::hasAlarm(uint8_t* deviceAddress)
{
  ScratchPad scratchPad;
  if (isConnected(deviceAddress, scratchPad))
  {
    float temp = calculateTemperature(deviceAddress, scratchPad);

    // check low alarm
    if ((char)temp <= (char)scratchPad[LOW_ALARM_TEMP]) return true;

    // check high alarm
    if ((char)temp >= (char)scratchPad[HIGH_ALARM_TEMP]) return true;
  }

  // no alarm
  return false;
}

// returns true if any device is reporting an alarm condition on the bus
bool DallasTemperature::hasAlarm(void)
{
  DeviceAddress deviceAddress;
  resetAlarmSearch();
  return alarmSearch(deviceAddress);
}

// runs the alarm handler for all devices returned by alarmSearch()
void DallasTemperature::processAlarms(void)
{
  resetAlarmSearch();
  DeviceAddress alarmAddr;

  while (alarmSearch(alarmAddr))
  {
    if (validAddress(alarmAddr))
      _AlarmHandler(alarmAddr);
  }
}

// sets the alarm handler
void DallasTemperature::setAlarmHandler(AlarmHandler *handler)
{
  _AlarmHandler = handler;
}

// The default alarm handler
void DallasTemperature::defaultAlarmHandler(uint8_t* deviceAddress)
{
}

#endif

// Convert float celsius to fahrenheit
float DallasTemperature::toFahrenheit(float celsius)
{
  return (celsius * 1.8) + 32;
}

// Convert float fahrenheit to celsius
float DallasTemperature::toCelsius(float fahrenheit)
{
  return (fahrenheit - 32) / 1.8;
}

#if REQUIRESNEW

// MnetCS - Allocates memory for DallasTemperature. Allows us to instance a new object
void* DallasTemperature::operator new(unsigned int size) // Implicit NSS obj size
{
  void * p; // void pointer
  p = malloc(size); // Allocate memory
  memset((DallasTemperature*)p,0,size); // Initalise memory

  //!!! CANT EXPLICITLY CALL CONSTRUCTOR - workaround by using an init() methodR - workaround by using an init() method
  return (DallasTemperature*) p; // Cast blank region to NSS pointer
}

// MnetCS 2009 -  Unallocates the memory used by this instance
void DallasTemperature::operator delete(void* p)
{
  DallasTemperature* pNss =  (DallasTemperature*) p; // Cast to NSS pointer
  pNss->~DallasTemperature(); // Destruct the object

  free(p); // Free the memory
}

#endif
