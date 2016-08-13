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
extern NodeConfig _nc;

// local variables
SPIFlash _flash(MY_OTA_FLASH_SS, MY_OTA_FLASH_JDECID);
NodeFirmwareConfig _fc;
bool _fwUpdateOngoing;
unsigned long _fwLastRequestTime;
uint16_t _fwBlock;
uint8_t _fwRetry;

void readFirmwareSettings() {
	hwReadConfigBlock((void*)&_fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(NodeFirmwareConfig));
}

void firmwareOTAUpdateRequest() {
	unsigned long enter = hwMillis();
	if (_fwUpdateOngoing && (enter - _fwLastRequestTime > MY_OTA_RETRY_DELAY)) {
		if (!_fwRetry) {
            setIndication(INDICATION_ERR_FW_TIMEOUT);
			OTA_DEBUG(PSTR("!OTA:FRQ:FW UPD FAIL\n"));	// fw update failed
			// Give up. We have requested MY_OTA_RETRY times without any packet in return.
			_fwUpdateOngoing = false;
			return;
		}
		_fwRetry--;
		_fwLastRequestTime = enter;
		// Time to (re-)request firmware block from controller
		RequestFWBlock firmwareRequest;
		firmwareRequest.type = _fc.type;
		firmwareRequest.version = _fc.version;
		firmwareRequest.block = (_fwBlock - 1);
		OTA_DEBUG(PSTR("OTA:FRQ:FW REQ,T=%04X,V=%04X,B=%04X\n"),_fc.type,_fc.version,_fwBlock - 1); // request FW update block
		_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_STREAM, ST_FIRMWARE_REQUEST, false).set(&firmwareRequest,sizeof(RequestFWBlock)));
	}
}

bool firmwareOTAUpdateProcess() {
	if (_msg.type == ST_FIRMWARE_CONFIG_RESPONSE) {
		NodeFirmwareConfig *firmwareConfigResponse = (NodeFirmwareConfig *)_msg.data;
		// compare with current node configuration, if they differ, start fw fetch process
		if (memcmp(&_fc,firmwareConfigResponse,sizeof(NodeFirmwareConfig))) {
            setIndication(INDICATION_FW_UPDATE_START);
			OTA_DEBUG(PSTR("OTA:FWP:UPDATE\n"));	// FW update initiated
			// copy new FW config
			memcpy(&_fc,firmwareConfigResponse,sizeof(NodeFirmwareConfig));
			// Init flash
			if (!_flash.initialize()) {
                setIndication(INDICATION_ERR_FW_FLASH_INIT);
				OTA_DEBUG(PSTR("!OTA:FWP:FLASH INIT FAIL\n"));	// failed to initialise flash
				_fwUpdateOngoing = false;
			} else {
				// erase lower 32K -> max flash size for ATMEGA328
				_flash.blockErase32K(0);
				// wait until flash erased
				while ( _flash.busy() );
				_fwBlock = _fc.blocks;
				_fwUpdateOngoing = true;
				// reset flags
				_fwRetry = MY_OTA_RETRY+1;
				_fwLastRequestTime = 0;
			}
			return true;
		}
		OTA_DEBUG(PSTR("OTA:FWP:UPDATE SKIPPED\n"));		// FW update skipped, no newer version available
	} else if (_msg.type == ST_FIRMWARE_RESPONSE) {
		if (_fwUpdateOngoing) {
			// Save block to flash
            setIndication(INDICATION_FW_UPDATE_RX);
			OTA_DEBUG(PSTR("OTA:FWP:RECV B=%04X\n"), _fwBlock);	// received FW block
			// extract FW block
			ReplyFWBlock *firmwareResponse = (ReplyFWBlock *)_msg.data;
			// write to flash
			_flash.writeBytes( ((_fwBlock - 1) * FIRMWARE_BLOCK_SIZE) + FIRMWARE_START_OFFSET, firmwareResponse->data, FIRMWARE_BLOCK_SIZE);
			// wait until flash written
			while ( _flash.busy() );
			_fwBlock--;
			if (!_fwBlock) {
				// We're finished! Do a checksum and reboot.
				OTA_DEBUG(PSTR("OTA:FWP:FW END\n"));	// received FW block
				_fwUpdateOngoing = false;
				if (transportIsValidFirmware()) {
					OTA_DEBUG(PSTR("OTA:FWP:CRC OK\n"));	// FW checksum ok
					// All seems ok, write size and signature to flash (DualOptiboot will pick this up and flash it)
					uint16_t fwsize = FIRMWARE_BLOCK_SIZE * _fc.blocks;
					uint8_t OTAbuffer[10] = {'F','L','X','I','M','G',':',(uint8_t)(fwsize >> 8),(uint8_t)(fwsize & 0xff),':'};
					_flash.writeBytes(0, OTAbuffer, 10);
					// Write the new firmware config to eeprom
					hwWriteConfigBlock((void*)&_fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(NodeFirmwareConfig));
					hwReboot();
				} else {
                    setIndication(INDICATION_ERR_FW_CHECKSUM);
					OTA_DEBUG(PSTR("!OTA:FWP:CRC FAIL\n"));
				}
			}
			// reset flags
			_fwRetry = MY_OTA_RETRY+1;
			_fwLastRequestTime = 0;
		} else {
			OTA_DEBUG(PSTR("!OTA:FWP:NO UPDATE\n"));
		}
		return true;
	}
	return false;
}

void presentBootloaderInformation(){
	RequestFirmwareConfig *reqFWConfig = (RequestFirmwareConfig *)_msgTmp.data;
	mSetLength(_msgTmp, sizeof(RequestFirmwareConfig));
	mSetCommand(_msgTmp, C_STREAM);
	mSetPayloadType(_msgTmp,P_CUSTOM);
	// copy node settings to reqFWConfig
	memcpy(reqFWConfig,&_fc,sizeof(NodeFirmwareConfig));
	// add bootloader information
	reqFWConfig->BLVersion = MY_OTA_BOOTLOADER_VERSION;
	_fwUpdateOngoing = false;
	_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_STREAM, ST_FIRMWARE_CONFIG_REQUEST, false));	
}
// do a crc16 on the whole received firmware
bool transportIsValidFirmware() {
	// init crc
	uint16_t crc = ~0;
	for (uint16_t i = 0; i < _fc.blocks * FIRMWARE_BLOCK_SIZE; ++i) {
		crc ^= _flash.readByte(i + FIRMWARE_START_OFFSET);
	    for (int8_t j = 0; j < 8; ++j) {
	        if (crc & 1)
	            crc = (crc >> 1) ^ 0xA001;
	        else
	            crc = (crc >> 1);
	    }
	}
	return crc == _fc.crc;
}
