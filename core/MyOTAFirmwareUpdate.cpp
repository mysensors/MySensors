/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2016 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyOTAFirmwareUpdate.h"

// global variables
extern MyMessage _msg;
extern MyMessage _msgTmp;

// local variables
SPIFlash _flash(MY_OTA_FLASH_SS, MY_OTA_FLASH_JDECID);
nodeFirmwareConfig_t _nodeFirmwareConfig;
bool _firmwareUpdateOngoing;
uint32_t _firmwareLastRequest;
uint16_t _firmwareBlock;
uint8_t _firmwareRetry;

void readFirmwareSettings(void)
{
	hwReadConfigBlock((void*)&_nodeFirmwareConfig, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS,
	                  sizeof(nodeFirmwareConfig_t));
}

void firmwareOTAUpdateRequest(void)
{
	const uint32_t enterMS = hwMillis();
	if (_firmwareUpdateOngoing && (enterMS - _firmwareLastRequest > MY_OTA_RETRY_DELAY)) {
		if (!_firmwareRetry) {
			setIndication(INDICATION_ERR_FW_TIMEOUT);
			OTA_DEBUG(PSTR("!OTA:FRQ:FW UPD FAIL\n"));	// fw update failed
			// Give up. We have requested MY_OTA_RETRY times without any packet in return.
			_firmwareUpdateOngoing = false;
			return;
		}
		_firmwareRetry--;
		_firmwareLastRequest = enterMS;
		// Time to (re-)request firmware block from controller
		requestFirmwareBlock_t firmwareRequest;
		firmwareRequest.type = _nodeFirmwareConfig.type;
		firmwareRequest.version = _nodeFirmwareConfig.version;
		firmwareRequest.block = (_firmwareBlock - 1);
		OTA_DEBUG(PSTR("OTA:FRQ:FW REQ,T=%04X,V=%04X,B=%04X\n"), _nodeFirmwareConfig.type,
		          _nodeFirmwareConfig.version, _firmwareBlock - 1); // request FW update block
		(void)_sendRoute(build(_msgTmp, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_STREAM, ST_FIRMWARE_REQUEST,
		                       false).set(&firmwareRequest, sizeof(requestFirmwareBlock_t)));
	}
}

bool firmwareOTAUpdateProcess(void)
{
	if (_msg.type == ST_FIRMWARE_CONFIG_RESPONSE) {
		nodeFirmwareConfig_t *firmwareConfigResponse = (nodeFirmwareConfig_t *)_msg.data;
		// compare with current node configuration, if they differ, start FW fetch process
		if (memcmp(&_nodeFirmwareConfig, firmwareConfigResponse, sizeof(nodeFirmwareConfig_t))) {
			setIndication(INDICATION_FW_UPDATE_START);
			OTA_DEBUG(PSTR("OTA:FWP:UPDATE\n"));	// FW update initiated
			// copy new FW config
			(void)memcpy(&_nodeFirmwareConfig, firmwareConfigResponse, sizeof(nodeFirmwareConfig_t));
			// Init flash
			if (!_flash.initialize()) {
				setIndication(INDICATION_ERR_FW_FLASH_INIT);
				OTA_DEBUG(PSTR("!OTA:FWP:FLASH INIT FAIL\n"));	// failed to initialise flash
				_firmwareUpdateOngoing = false;
			} else {
				// erase lower 32K -> max flash size for ATMEGA328
				_flash.blockErase32K(0);
				// wait until flash erased
				while ( _flash.busy() ) {}
				_firmwareBlock = _nodeFirmwareConfig.blocks;
				_firmwareUpdateOngoing = true;
				// reset flags
				_firmwareRetry = MY_OTA_RETRY + 1;
				_firmwareLastRequest = 0;
			}
			return true;
		}
		OTA_DEBUG(PSTR("OTA:FWP:UPDATE SKIPPED\n"));		// FW update skipped, no newer version available
	} else if (_msg.type == ST_FIRMWARE_RESPONSE) {
		if (_firmwareUpdateOngoing) {
			// extract FW block
			replyFirmwareBlock_t *firmwareResponse = (replyFirmwareBlock_t *)_msg.data;

			OTA_DEBUG(PSTR("OTA:FWP:RECV B=%04X\n"), firmwareResponse->block);	// received FW block
			if (firmwareResponse->block != _firmwareBlock - 1) {
				OTA_DEBUG(PSTR("!OTA:FWP:WRONG FWB\n"));	// received FW block
				// wrong firmware block received
				setIndication(INDICATION_FW_UPDATE_RX_ERR);
				// no further processing required
				return true;
			}
			setIndication(INDICATION_FW_UPDATE_RX);
			// Save block to flash
			_flash.writeBytes( ((_firmwareBlock - 1) * FIRMWARE_BLOCK_SIZE) + FIRMWARE_START_OFFSET,
			                   firmwareResponse->data, FIRMWARE_BLOCK_SIZE);
			// wait until flash written
			while (_flash.busy()) {}
			_firmwareBlock--;
			if (!_firmwareBlock) {
				// We're done! Do a checksum and reboot.
				OTA_DEBUG(PSTR("OTA:FWP:FW END\n"));	// received FW block
				_firmwareUpdateOngoing = false;
				if (transportIsValidFirmware()) {
					OTA_DEBUG(PSTR("OTA:FWP:CRC OK\n"));	// FW checksum ok
					// Write the new firmware config to eeprom
					hwWriteConfigBlock((void*)&_nodeFirmwareConfig, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS,
					                   sizeof(nodeFirmwareConfig_t));
					// All seems ok, write size and signature to flash (DualOptiboot will pick this up and flash it)
					const uint16_t firmwareSize = FIRMWARE_BLOCK_SIZE * _nodeFirmwareConfig.blocks;
					const uint8_t OTAbuffer[FIRMWARE_START_OFFSET] = {'F','L','X','I','M','G',':', (uint8_t)(firmwareSize >> 8), (uint8_t)(firmwareSize & 0xff),':'};
					_flash.writeBytes(0, OTAbuffer, FIRMWARE_START_OFFSET);
					// wait until flash ready
					while (_flash.busy()) {}
					hwReboot();
				} else {
					setIndication(INDICATION_ERR_FW_CHECKSUM);
					OTA_DEBUG(PSTR("!OTA:FWP:CRC FAIL\n"));
				}
			}
			// reset flags
			_firmwareRetry = MY_OTA_RETRY + 1;
			_firmwareLastRequest = 0;
		} else {
			OTA_DEBUG(PSTR("!OTA:FWP:NO UPDATE\n"));
		}
		return true;
	}
	return false;
}

void presentBootloaderInformation(void)
{
	requestFirmwareConfig_t *requestFirmwareConfig = (requestFirmwareConfig_t *)_msgTmp.data;
	mSetLength(_msgTmp, sizeof(requestFirmwareConfig_t));
	mSetCommand(_msgTmp, C_STREAM);
	mSetPayloadType(_msgTmp, P_CUSTOM);
	// copy node settings to reqFWConfig
	(void)memcpy(requestFirmwareConfig, &_nodeFirmwareConfig, sizeof(nodeFirmwareConfig_t));
	// add bootloader information
	requestFirmwareConfig->BLVersion = MY_OTA_BOOTLOADER_VERSION;
	_firmwareUpdateOngoing = false;
	(void)_sendRoute(build(_msgTmp, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_STREAM,
	                       ST_FIRMWARE_CONFIG_REQUEST, false));
}
bool isFirmwareUpdateOngoing(void)
{
	return _firmwareUpdateOngoing;
}
// do a crc16 on the whole received firmware
bool transportIsValidFirmware(void)
{
	// init crc
	uint16_t crc = ~0;
	for (uint16_t i = 0; i < _nodeFirmwareConfig.blocks * FIRMWARE_BLOCK_SIZE; ++i) {
		crc ^= _flash.readByte(i + FIRMWARE_START_OFFSET);
		for (int8_t j = 0; j < 8; ++j) {
			if (crc & 1) {
				crc = (crc >> 1) ^ 0xA001;
			} else {
				crc = (crc >> 1);
			}
		}
	}
	OTA_DEBUG(PSTR("OTA:CRC:B=%04X,C=%04X,F=%04X\n"), _nodeFirmwareConfig.blocks,crc,
	          _nodeFirmwareConfig.crc);
	return crc == _nodeFirmwareConfig.crc;
}
