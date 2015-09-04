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
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Patrick Fallberg, January 10, 2015
 * 
 * DESCRIPTION
 * SHA204 personalization sketch
 *
 * This sketch will write factory default settings to the configuration zone
 * and then lock it.
 * It will then either
 * a. Generate a random value to use as a key which will be stored in
 * slot 0. The key is printed on UART (115200) in clear text for the user to be
 * able to use it as a user-supplied key in other personalization executions
 * where the same key is needed.
 * b. Use a user-supplied value to use as a key which will be stored in
 * slot 0.
 * Finally it will lock the data zone.
 *
 * By default, no locking is performed. User have to manually enable the flags that
 * turn on the locking. Furthermore, user have to send a SPACE character on serial
 * console when prompted to do any locking. On boards that does not provide UART
 * input it is possible to configure the sketch to skip this confirmation.
 * Default settings use ATSHA204 on pin 17 (A3).
 */

#include <sha204_library.h>
#include <sha204_lib_return_codes.h>

// The pin the ATSHA204 is connected on
#define ATSHA204_PIN 17 // A3

// Uncomment this to enable locking the configuration zone.
// *** BE AWARE THAT THIS PREVENTS ANY FUTURE CONFIGURATION CHANGE TO THE CHIP ***
// It is still possible to change the key, and this also enable random key generation
//#define LOCK_CONFIGURATION

// Uncomment this to enable locking the data zone.
// *** BE AWARE THAT THIS PREVENTS THE KEY TO BE CHANGED ***
// It is not required to lock data, key cannot be retrieved anyway, but by locking
// data, it can be guaranteed that nobody even with physical access to the chip,
// will be able to change the key.
//#define LOCK_DATA

// Uncomment this to skip key storage (typically once key has been written once)
//#define SKIP_KEY_STORAGE

// Uncomment this to skip key data storage (once configuration is locked, key
// will aways randomize)
// Uncomment this to skip key generation and use 'user_key_data' as key instead.
//#define USER_KEY_DATA

// Uncomment this for boards that lack UART
// IMPORTANT: No confirmation will be required for locking any zones with this
// configuration!
// Also, key generation is not permitted in this mode as there is no way of
// presenting the generated key.
//#define SKIP_UART_CONFIRMATION

#if defined(SKIP_UART_CONFIRMATION) && !defined(USER_KEY_DATA)
#error You have to define USER_KEY_DATA for boards that does not have UART
#endif

#ifdef USER_KEY_DATA
#define MY_HMAC_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
const uint8_t user_key_data[32] = {MY_HMAC_KEY};
#endif

const int sha204Pin = ATSHA204_PIN;
atsha204Class sha204(sha204Pin);

void halt()
{
  Serial.println(F("Halting!"));
  while(1);
}

uint16_t write_config_and_get_crc()
{
  uint16_t crc = 0;
  uint8_t config_word[4];
  uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;
  bool do_write;

  // We will set default settings from datasheet on all slots. This means that we can use slot 0 for the key
  // as that slot will not be readable (key will therefore be secure) and slot 8 for the payload digest
  // calculationon as that slot can be written in clear text even when the datazone is locked.
  // Other settings which are not relevant are kept as is.

  for (int i=0; i < 88; i += 4)
  {
    do_write = true;
    if (i == 20)
    {
      config_word[0] = 0x8F;
      config_word[1] = 0x80;
      config_word[2] = 0x80;
      config_word[3] = 0xA1;
    }
    else if (i == 24)
    {
      config_word[0] = 0x82;
      config_word[1] = 0xE0;
      config_word[2] = 0xA3;
      config_word[3] = 0x60;
    }
    else if (i == 28)
    {
      config_word[0] = 0x94;
      config_word[1] = 0x40;
      config_word[2] = 0xA0;
      config_word[3] = 0x85;
    }
    else if (i == 32)
    {
      config_word[0] = 0x86;
      config_word[1] = 0x40;
      config_word[2] = 0x87;
      config_word[3] = 0x07;
    }
    else if (i == 36)
    {
      config_word[0] = 0x0F;
      config_word[1] = 0x00;
      config_word[2] = 0x89;
      config_word[3] = 0xF2;
    }
    else if (i == 40)
    {
      config_word[0] = 0x8A;
      config_word[1] = 0x7A;
      config_word[2] = 0x0B;
      config_word[3] = 0x8B;
    }
    else if (i == 44)
    {
      config_word[0] = 0x0C;
      config_word[1] = 0x4C;
      config_word[2] = 0xDD;
      config_word[3] = 0x4D;
    }
    else if (i == 48)
    {
      config_word[0] = 0xC2;
      config_word[1] = 0x42;
      config_word[2] = 0xAF;
      config_word[3] = 0x8F;
    }
    else if (i == 52 || i == 56 || i == 60 || i == 64)
    {
      config_word[0] = 0xFF;
      config_word[1] = 0x00;
      config_word[2] = 0xFF;
      config_word[3] = 0x00;
    }
    else if (i == 68 || i == 72 || i == 76 || i == 80)
    {
      config_word[0] = 0xFF;
      config_word[1] = 0xFF;
      config_word[2] = 0xFF;
      config_word[3] = 0xFF;
    }
    else
    {
      // All other configs are untouched
      ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, i);
      if (ret_code != SHA204_SUCCESS)
      {
        Serial.print(F("Failed to read config. Response: ")); Serial.println(ret_code, HEX);
        halt();
      }
      // Set config_word to the read data
      config_word[0] = rx_buffer[SHA204_BUFFER_POS_DATA+0];
      config_word[1] = rx_buffer[SHA204_BUFFER_POS_DATA+1];
      config_word[2] = rx_buffer[SHA204_BUFFER_POS_DATA+2];
      config_word[3] = rx_buffer[SHA204_BUFFER_POS_DATA+3];
      do_write = false;
    }

    // Update crc with CRC for the current word
    crc = sha204.calculateAndUpdateCrc(4, config_word, crc);

    // Write config word
    if (do_write)
    {
      ret_code = sha204.sha204m_execute(SHA204_WRITE, SHA204_ZONE_CONFIG,
                                        i >> 2, 4, config_word, 0, NULL, 0, NULL,
                                        WRITE_COUNT_SHORT, tx_buffer, WRITE_RSP_SIZE, rx_buffer);
      if (ret_code != SHA204_SUCCESS)
      {
        Serial.print(F("Failed to write config word at address ")); Serial.print(i);
        Serial.print(F(". Response: ")); Serial.println(ret_code, HEX);
        halt();
      }
    }
  }
  return crc;
}

void write_key(uint8_t* key)
{
  uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;

  // Write key to slot 0
  ret_code = sha204.sha204m_execute(SHA204_WRITE, SHA204_ZONE_DATA | SHA204_ZONE_COUNT_FLAG,
                                    0, SHA204_ZONE_ACCESS_32, key, 0, NULL, 0, NULL,
                                    WRITE_COUNT_LONG, tx_buffer, WRITE_RSP_SIZE, rx_buffer);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print(F("Failed to write key to slot 0. Response: ")); Serial.println(ret_code, HEX);
    halt();
  }
}

void dump_configuration()
{
  uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;

  for (int i=0; i < 88; i += 4)
  {
    ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, i);
    if (ret_code != SHA204_SUCCESS)
    {
      Serial.print(F("Failed to read config. Response: ")); Serial.println(ret_code, HEX);
      halt();
    }
    if (i == 0x00)
    {
      Serial.print(F("           SN[0:1]           |         SN[2:3]           | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x04)
    {
      Serial.print(F("                          Revnum                         | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print(F("   "));
      }
      Serial.println();
    }
    else if (i == 0x08)
    {
      Serial.print(F("                          SN[4:7]                        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print(F("   "));
      }
      Serial.println();
    }
    else if (i == 0x0C)
    {
      Serial.print(F("    SN[8]    |  Reserved13   | I2CEnable | Reserved15    | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x10)
    {
      Serial.print(F("  I2CAddress |  TempOffset   |  OTPmode  | SelectorMode  | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x14)
    {
      Serial.print(F("         SlotConfig00        |       SlotConfig01        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x18)
    {
      Serial.print(F("         SlotConfig02        |       SlotConfig03        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x1C)
    {
      Serial.print(F("         SlotConfig04        |       SlotConfig05        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x20)
    {
      Serial.print(F("         SlotConfig06        |       SlotConfig07        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x24)
    {
      Serial.print(F("         SlotConfig08        |       SlotConfig09        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x28)
    {
      Serial.print(F("         SlotConfig0A        |       SlotConfig0B        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x2C)
    {
      Serial.print(F("         SlotConfig0C        |       SlotConfig0D        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x30)
    {
      Serial.print(F("         SlotConfig0E        |       SlotConfig0F        | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j == 1)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x34)
    {
      Serial.print(F("  UseFlag00  | UpdateCount00 | UseFlag01 | UpdateCount01 | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x38)
    {
      Serial.print(F("  UseFlag02  | UpdateCount02 | UseFlag03 | UpdateCount03 | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x3C)
    {
      Serial.print(F("  UseFlag04  | UpdateCount04 | UseFlag05 | UpdateCount05 | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x40)
    {
      Serial.print(F("  UseFlag06  | UpdateCount06 | UseFlag07 | UpdateCount07 | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
    else if (i == 0x44)
    {
      Serial.print(F("                      LastKeyUse[0:3]                    | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print(F("   "));
      }
      Serial.println();
    }
    else if (i == 0x48)
    {
      Serial.print(F("                      LastKeyUse[4:7]                    | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print(F("   "));
      }
      Serial.println();
    }
    else if (i == 0x4C)
    {
      Serial.print(F("                      LastKeyUse[8:B]                    | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print(F("   "));
      }
      Serial.println();
    }
    else if (i == 0x50)
    {
      Serial.print(F("                      LastKeyUse[C:F]                    | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        Serial.print(F("   "));
      }
      Serial.println();
    }
    else if (i == 0x54)
    {
      Serial.print(F("  UserExtra  |    Selector   | LockValue |  LockConfig   | "));
      for (int j=0; j<4; j++)
      {
        if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
        if (j < 3)
        {
          Serial.print(F(" | "));
        }
        else
        {
          Serial.print(F("   "));
        }
      }
      Serial.println();
    }
  }
}

void setup()
{
  uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t key[32];
  uint8_t ret_code;
  uint8_t lockConfig = 0;
  uint8_t lockValue = 0;
  uint16_t crc;
  (void)crc;

  Serial.begin(115200);

  Serial.println(F("ATSHA204 personalization sketch for MySensors usage."));
  Serial.println(F("----------------------------------------------------"));

  // Wake device before starting operations
  ret_code = sha204.sha204c_wakeup(rx_buffer);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print(F("Failed to wake device. Response: ")); Serial.println(ret_code, HEX);
    halt();
  }

  // Output device revision on console
  ret_code = sha204.sha204m_dev_rev(tx_buffer, rx_buffer);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print(F("Failed to determine device revision. Response: ")); Serial.println(ret_code, HEX);
    halt();
  }
  else
  {
    Serial.print(F("Device revision: "));
    for (int i=0; i<4; i++)
    {
      if (rx_buffer[SHA204_BUFFER_POS_DATA+i] < 0x10)
      {
        Serial.print('0'); // Because Serial.print does not 0-pad HEX
      }
      Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+i], HEX);
    }
    Serial.println();
  }

  // Output serial number on console
  ret_code = sha204.getSerialNumber(rx_buffer);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print(F("Failed to obtain device serial number. Response: ")); Serial.println(ret_code, HEX);
    halt();
  }
  else
  {
    Serial.print(F("Device serial:   "));
    Serial.print('{');
    for (int i=0; i<9; i++)
    {
      Serial.print(F("0x"));
      if (rx_buffer[i] < 0x10)
      {
        Serial.print('0'); // Because Serial.print does not 0-pad HEX
      }
      Serial.print(rx_buffer[i], HEX);
      if (i < 8) Serial.print(',');
    }
    Serial.print('}');
    Serial.println();
    for (int i=0; i<9; i++)
    {
      if (rx_buffer[i] < 0x10)
      {
        Serial.print('0'); // Because Serial.print does not 0-pad HEX
      }
      Serial.print(rx_buffer[i], HEX);
    }
    Serial.println();
  }

  // Read out lock config bits to determine if locking is possible
  ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print(F("Failed to determine device lock status. Response: ")); Serial.println(ret_code, HEX);
    halt();
  }
  else
  {
    lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
    lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
  }

  if (lockConfig != 0x00)
  {
    // Write config and get CRC for the updated config
    crc = write_config_and_get_crc();

    // List current configuration before attempting to lock
    Serial.println(F("Chip configuration:"));
    dump_configuration();

#ifdef LOCK_CONFIGURATION
    // Purge serial input buffer
#ifndef SKIP_UART_CONFIRMATION
    while (Serial.available())
    {
      Serial.read();
    }
    Serial.println(F("Send SPACE character now to lock the configuration..."));

    while (Serial.available() == 0);
    if (Serial.read() == ' ')
#endif //not SKIP_UART_CONFIRMATION
    {
      Serial.println(F("Locking configuration..."));

      // Correct sequence, resync chip
      ret_code = sha204.sha204c_resync(SHA204_RSP_SIZE_MAX, rx_buffer);
      if (ret_code != SHA204_SUCCESS && ret_code != SHA204_RESYNC_WITH_WAKEUP)
      {
        Serial.print(F("Resync failed. Response: ")); Serial.println(ret_code, HEX);
        halt();
      }

      // Lock configuration zone
      ret_code = sha204.sha204m_execute(SHA204_LOCK, SHA204_ZONE_CONFIG,
                                        crc, 0, NULL, 0, NULL, 0, NULL,
                                        LOCK_COUNT, tx_buffer, LOCK_RSP_SIZE, rx_buffer);
      if (ret_code != SHA204_SUCCESS)
      {
        Serial.print(F("Configuration lock failed. Response: ")); Serial.println(ret_code, HEX);
        halt();
      }
      else
      {
        Serial.println(F("Configuration locked."));

        // Update lock flags after locking
        ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
        if (ret_code != SHA204_SUCCESS)
        {
          Serial.print(F("Failed to determine device lock status. Response: ")); Serial.println(ret_code, HEX);
          halt();
        }
        else
        {
          lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
          lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
        }
      }
    }
#ifndef SKIP_UART_CONFIRMATION
    else
    {
      Serial.println(F("Unexpected answer. Skipping lock."));
    }
#endif //not SKIP_UART_CONFIRMATION
#else //LOCK_CONFIGURATION
    Serial.println(F("Configuration not locked. Define LOCK_CONFIGURATION to lock for real."));
#endif
  }
  else
  {
    Serial.println(F("Skipping configuration write and lock (configuration already locked)."));
    Serial.println(F("Chip configuration:"));
    dump_configuration();
  }

#ifdef SKIP_KEY_STORAGE
  Serial.println(F("Disable SKIP_KEY_STORAGE to store key."));
#else
#ifdef USER_KEY_DATA
  memcpy(key, user_key_data, 32);
  Serial.println(F("Using this user supplied key:"));
#else
  // Retrieve random value to use as key
  ret_code = sha204.sha204m_random(tx_buffer, rx_buffer, RANDOM_SEED_UPDATE);
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print(F("Random key generation failed. Response: ")); Serial.println(ret_code, HEX);
    halt();
  }
  else
  {
    memcpy(key, rx_buffer+SHA204_BUFFER_POS_DATA, 32);
  }
  if (lockConfig == 0x00)
  {
    Serial.println(F("Take note of this key, it will never be the shown again:"));
  }
  else
  {
    Serial.println(F("Key is not randomized (configuration not locked):"));
  }
#endif
  Serial.print("#define MY_HMAC_KEY ");
  for (int i=0; i<32; i++)
  {
    Serial.print("0x");
    if (key[i] < 0x10)
    {
      Serial.print('0'); // Because Serial.print does not 0-pad HEX
    }
    Serial.print(key[i], HEX);
    if (i < 31) Serial.print(',');
    if (i+1 == 16) Serial.print("\\\n                    ");
  }
  Serial.println();

  // It will not be possible to write the key if the configuration zone is unlocked
  if (lockConfig == 0x00)
  {
    // Write the key to the appropriate slot in the data zone
    Serial.println(F("Writing key to slot 0..."));
    write_key(key);
  }
  else
  {
    Serial.println(F("Skipping key storage (configuration not locked)."));
    Serial.println(F("The configuration must be locked to be able to write a key."));
  }  
#endif

  if (lockValue != 0x00)
  {
#ifdef LOCK_DATA
#ifndef SKIP_UART_CONFIRMATION
    while (Serial.available())
    {
      Serial.read();
    }
    Serial.println(F("Send SPACE character to lock data..."));
    while (Serial.available() == 0);
    if (Serial.read() == ' ')
#endif //not SKIP_UART_CONFIRMATION
    {
      // Correct sequence, resync chip
      ret_code = sha204.sha204c_resync(SHA204_RSP_SIZE_MAX, rx_buffer);
      if (ret_code != SHA204_SUCCESS && ret_code != SHA204_RESYNC_WITH_WAKEUP)
      {
        Serial.print(F("Resync failed. Response: ")); Serial.println(ret_code, HEX);
        halt();
      }

      // If configuration is unlocked, key is not updated. Locking data in this case will cause
      // slot 0 to contain an unknown (or factory default) key, and this is in practically any
      // usecase not the desired behaviour, so ask for additional confirmation in this case.
      if (lockConfig != 0x00)
      {
        while (Serial.available())
        {
          Serial.read();
        }
        Serial.println(F("*** ATTENTION ***"));
        Serial.println(F("Configuration is not locked. Are you ABSULOUTELY SURE you want to lock data?"));
        Serial.println(F("Locking data at this stage will cause slot 0 to contain a factory default key"));
        Serial.println(F("which cannot be change after locking is done. This is in practically any usecase"));
        Serial.println(F("NOT the desired behavour. Send SPACE character now to lock data anyway..."));
        while (Serial.available() == 0);
        if (Serial.read() != ' ')
        {
          Serial.println(F("Unexpected answer. Skipping lock."));
          halt();
        }
      }

      // Lock data zone
      ret_code = sha204.sha204m_execute(SHA204_LOCK, SHA204_ZONE_DATA | LOCK_ZONE_NO_CRC,
                                        0x0000, 0, NULL, 0, NULL, 0, NULL,
                                        LOCK_COUNT, tx_buffer, LOCK_RSP_SIZE, rx_buffer);
      if (ret_code != SHA204_SUCCESS)
      {
        Serial.print(F("Data lock failed. Response: ")); Serial.println(ret_code, HEX);
        halt();
      }
      else
      {
        Serial.println(F("Data locked."));

        // Update lock flags after locking
        ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
        if (ret_code != SHA204_SUCCESS)
        {
          Serial.print(F("Failed to determine device lock status. Response: ")); Serial.println(ret_code, HEX);
          halt();
        }
        else
        {
          lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
          lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
        }
      }
    }
#ifndef SKIP_UART_CONFIRMATION
    else
    {
      Serial.println(F("Unexpected answer. Skipping lock."));
    }
#endif //not SKIP_UART_CONFIRMATION
#else //LOCK_DATA
    Serial.println(F("Data not locked. Define LOCK_DATA to lock for real."));
#endif
  }
  else
  {
    Serial.println(F("Skipping OTP/data zone lock (zone already locked)."));
  }

  Serial.println(F("--------------------------------"));
  Serial.println(F("Personalization is now complete."));
  Serial.print(F("Configuration is "));
  if (lockConfig == 0x00)
  {
    Serial.println("LOCKED");
  }
  else
  {
    Serial.println("UNLOCKED");
  }
  Serial.print(F("Data is "));
  if (lockValue == 0x00)
  {
    Serial.println("LOCKED");
  }
  else
  {
    Serial.println("UNLOCKED");
  }
}

void loop()
{
}
