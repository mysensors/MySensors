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
 */

#include "MyOTAFirmwareUpdate.h"

// global variables
extern MyMessage _msg;
extern MyMessage _msgTmp;

// local variables
#ifdef MY_OTA_USE_I2C_EEPROM
I2CEeprom _flash(MY_OTA_I2C_ADDR);
#elif !defined(MCUBOOT_PRESENT)
SPIFlash _flash(MY_OTA_FLASH_SS, MY_OTA_FLASH_JDECID);
#endif

// Map flash functions
#ifndef MCUBOOT_PRESENT
#define _flash_initialize()	_flash.initialize()
#define _flash_readByte(addr)	_flash.readByte(addr)
#define _flash_writeBytes( dstaddr, data, size) _flash.writeBytes( dstaddr, data, size)
#define  _flash_blockErase32K(num)  _flash.blockErase32K(num)
#define _flash_busy() _flash.busy()
#else
#define _flash_initialize()	true
#define _flash_readByte(addr)	(*((uint8_t *)(addr)))
#define  _flash_blockErase32K(num)  Flash.erase((uint32_t *)FLASH_AREA_IMAGE_1_OFFSET_0, FLASH_AREA_IMAGE_1_SIZE_0)
#define _flash_busy() false
#endif

LOCAL nodeFirmwareConfig_t _nodeFirmwareConfig;
LOCAL bool _firmwareUpdateOngoing = false;
LOCAL uint32_t _firmwareLastRequest;
LOCAL uint16_t _firmwareBlock;
LOCAL uint8_t _firmwareRetry;
LOCAL bool _firmwareResponse(uint16_t block, uint8_t *data);

LOCAL void readFirmwareSettings(void)
{
	hwReadConfigBlock((void*)&_nodeFirmwareConfig, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS,
	                  sizeof(nodeFirmwareConfig_t));
}

LOCAL void firmwareOTAUpdateRequest(void)
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
		OTA_DEBUG(PSTR("OTA:FRQ:FW REQ,T=%04" PRIX16 ",V=%04" PRIX16 ",B=%04" PRIX16 "\n"),
		          _nodeFirmwareConfig.type,
		          _nodeFirmwareConfig.version, _firmwareBlock - 1); // request FW update block
		(void)_sendRoute(build(_msgTmp, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_STREAM, ST_FIRMWARE_REQUEST,
		                       false).set(&firmwareRequest, sizeof(requestFirmwareBlock_t)));
	}
}

LOCAL bool firmwareOTAUpdateProcess(void)
{
	if (_msg.type == ST_FIRMWARE_CONFIG_RESPONSE) {
		if(_firmwareUpdateOngoing) {
			OTA_DEBUG(PSTR("!OTA:FWP:UPDO\n"));	// FW config response received, FW update already ongoing
			return true;
		}
		nodeFirmwareConfig_t *firmwareConfigResponse = (nodeFirmwareConfig_t *)_msg.data;
		// compare with current node configuration, if they differ, start FW fetch process
		if (memcmp(&_nodeFirmwareConfig, firmwareConfigResponse, sizeof(nodeFirmwareConfig_t))) {
			setIndication(INDICATION_FW_UPDATE_START);
			OTA_DEBUG(PSTR("OTA:FWP:UPDATE\n"));	// FW update initiated
			// copy new FW config
			(void)memcpy(&_nodeFirmwareConfig, firmwareConfigResponse, sizeof(nodeFirmwareConfig_t));
			// Init flash
			if (!_flash_initialize()) {
				setIndication(INDICATION_ERR_FW_FLASH_INIT);
				OTA_DEBUG(PSTR("!OTA:FWP:FLASH INIT FAIL\n"));	// failed to initialise flash
				_firmwareUpdateOngoing = false;
			} else {
				// erase lower 32K -> max flash size for ATMEGA328
				_flash_blockErase32K(0);
				// wait until flash erased
				while ( _flash_busy() ) {}
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
		// extract FW block
		replyFirmwareBlock_t *firmwareResponse = (replyFirmwareBlock_t *)_msg.data;
		// Proceed firmware data
		return _firmwareResponse(firmwareResponse->block, firmwareResponse->data);
#ifdef FIRMWARE_PROTOCOL_31
	} else if (_msg.type == ST_FIRMWARE_RESPONSE_RLE) {
		// RLE encoded block
		// extract FW block
		replyFirmwareBlockRLE_t *firmwareResponse = (replyFirmwareBlockRLE_t *)_msg.data;
		uint8_t data[FIRMWARE_BLOCK_SIZE];
		for (uint8_t i=0; i<FIRMWARE_BLOCK_SIZE; i++) {
			data[i]=firmwareResponse->data;
		}
		while ((_firmwareBlock) && (firmwareResponse->number_of_blocks)) {
			_firmwareResponse(firmwareResponse->block, data);
			firmwareResponse->number_of_blocks--;
			firmwareResponse->block--;
		}
		return true;
#endif
	} else {
#ifdef MCUBOOT_PRESENT
		if (_msg.type == ST_FIRMWARE_CONFIRM) {
			if (*(uint16_t *)MCUBOOT_IMAGE_0_MAGIC_ADDR == ((uint16_t)MCUBOOT_IMAGE_MAGIC)) {
				if (*(uint8_t*)(MCUBOOT_IMAGE_0_IMG_OK_ADDR)!=MCUBOOT_IMAGE_0_IMG_OK_BYTE) {
					// Calculate data word to write
					uint32_t *img_ok_base_addr = (uint32_t*)(MCUBOOT_IMAGE_0_IMG_OK_ADDR & ~3); // align word wise
					uint32_t img_ok_data = *img_ok_base_addr;
					// Set copy of MCUBOOT_IMAGE_0_IMG_OK_ADDR to MCUBOOT_IMAGE_0_IMG_OK_BYTE (0x01)
					uint8_t * img_ok_array = (uint8_t*)(&img_ok_data);
					img_ok_array[MCUBOOT_IMAGE_0_IMG_OK_ADDR % 4] = MCUBOOT_IMAGE_0_IMG_OK_BYTE;
					// Write word back
					Flash.write(img_ok_base_addr, img_ok_data);
				}
				OTA_DEBUG(PSTR("!OTA:FWP:IMAGE CONFIRMED\n"));
			} else {
				OTA_DEBUG(PSTR("!OTA:FWP:INVALID MCUBOOT MAGIC\n"));
			}
		}
#endif
	}
	return false;
}

LOCAL void presentBootloaderInformation(void)
{
	requestFirmwareConfig_t *requestFirmwareConfig = (requestFirmwareConfig_t *)_msgTmp.data;
	mSetLength(_msgTmp, sizeof(requestFirmwareConfig_t));
	mSetCommand(_msgTmp, C_STREAM);
	mSetPayloadType(_msgTmp, P_CUSTOM);
	// copy node settings to reqFWConfig
	(void)memcpy(requestFirmwareConfig, &_nodeFirmwareConfig, sizeof(nodeFirmwareConfig_t));
	// add bootloader information
	requestFirmwareConfig->BLVersion = MY_OTA_BOOTLOADER_VERSION;
#ifdef FIRMWARE_PROTOCOL_31
	requestFirmwareConfig->blockSize = FIRMWARE_BLOCK_SIZE;
#ifndef MCUBOOT_PRESENT
	requestFirmwareConfig->img_commited = 0x2;
	requestFirmwareConfig->img_revision = 0x00;
	requestFirmwareConfig->img_build_num = 0x00;
#else
	requestFirmwareConfig->img_commited = *((uint8_t*)(MCUBOOT_IMAGE_0_IMG_OK_ADDR));
	requestFirmwareConfig->img_revision = *((uint16_t*)(MCUBOOT_IMAGE_0_IMG_REVISION_ADDR));
	requestFirmwareConfig->img_build_num = *((uint16_t*)(MCUBOOT_IMAGE_0_IMG_BUILD_NUM_ADDR));
#endif
#endif
	_firmwareUpdateOngoing = false;
	(void)_sendRoute(build(_msgTmp, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_STREAM,
	                       ST_FIRMWARE_CONFIG_REQUEST, false));
}

LOCAL bool isFirmwareUpdateOngoing(void)
{
	return _firmwareUpdateOngoing;
}
// do a crc16 on the whole received firmware
LOCAL bool transportIsValidFirmware(void)
{
	// init crc
	uint16_t crc = ~0;
	for (uint32_t i = 0; i < _nodeFirmwareConfig.blocks * FIRMWARE_BLOCK_SIZE; ++i) {
		crc ^= _flash_readByte(i + FIRMWARE_START_OFFSET);
		for (int8_t j = 0; j < 8; ++j) {
			if (crc & 1) {
				crc = (crc >> 1) ^ 0xA001;
			} else {
				crc = (crc >> 1);
			}
		}
	}
	OTA_DEBUG(PSTR("OTA:CRC:B=%04" PRIX16 ",C=%04" PRIX16 ",F=%04" PRIX16 "\n"),
	          _nodeFirmwareConfig.blocks,crc,
	          _nodeFirmwareConfig.crc);
	return crc == _nodeFirmwareConfig.crc;
}

LOCAL bool _firmwareResponse(uint16_t block, uint8_t *data)
{
	if (_firmwareUpdateOngoing) {
		OTA_DEBUG(PSTR("OTA:FWP:RECV B=%04" PRIX16 "\n"), block);	// received FW block
		if (block != _firmwareBlock - 1) {
			OTA_DEBUG(PSTR("!OTA:FWP:WRONG FWB\n"));	// received FW block
			// wrong firmware block received
			setIndication(INDICATION_FW_UPDATE_RX_ERR);
			// no further processing required
			return true;
		}
		setIndication(INDICATION_FW_UPDATE_RX);
		// Save block to flash
#ifdef MCUBOOT_PRESENT
		uint32_t addr = ((size_t)(((_firmwareBlock - 1) * FIRMWARE_BLOCK_SIZE)) + (size_t)(
		                     FIRMWARE_START_OFFSET));
		if (addr<FLASH_AREA_IMAGE_SCRATCH_OFFSET_0) {
			Flash.write_block( (uint32_t *)addr, (uint32_t *)data, FIRMWARE_BLOCK_SIZE>>2);
		}
#else
		_flash_writeBytes( ((_firmwareBlock - 1) * FIRMWARE_BLOCK_SIZE) + FIRMWARE_START_OFFSET,
		                   data, FIRMWARE_BLOCK_SIZE);
#endif
		// wait until flash written
		while (_flash_busy()) {}
#ifdef OTA_EXTRA_FLASH_DEBUG
		{
			char prbuf[8];
			uint32_t addr = ((_firmwareBlock - 1) * FIRMWARE_BLOCK_SIZE) + FIRMWARE_START_OFFSET;
			OTA_DEBUG(PSTR("OTA:FWP:FL DUMP "));
			sprintf_P(prbuf,PSTR("%04" PRIX16 ":"), (uint16_t)addr);
			MY_SERIALDEVICE.print(prbuf);
			for(uint8_t i=0; i<FIRMWARE_BLOCK_SIZE; i++) {
				uint8_t data = _flash_readByte(addr + i);
				sprintf_P(prbuf,PSTR("%02" PRIX8 ""), (uint8_t)data);
				MY_SERIALDEVICE.print(prbuf);
			}
			OTA_DEBUG(PSTR("\n"));
		}
#endif
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
#ifndef MCUBOOT_PRESENT
				// All seems ok, write size and signature to flash (DualOptiboot will pick this up and flash it)
				const uint16_t firmwareSize = FIRMWARE_BLOCK_SIZE * _nodeFirmwareConfig.blocks;
				const uint8_t OTAbuffer[FIRMWARE_START_OFFSET] = {'F','L','X','I','M','G',':', (uint8_t)(firmwareSize >> 8), (uint8_t)(firmwareSize & 0xff),':'};
				_flash_writeBytes(0, OTAbuffer, FIRMWARE_START_OFFSET);
				// wait until flash ready
				while (_flash_busy()) {}
#endif
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
