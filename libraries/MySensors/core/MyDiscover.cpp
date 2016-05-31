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

#include "MyDiscover.h"

void generateDiscoverResponse(uint8_t page, uint8_t* buffer, uint8_t *len) {
	MyDiscover *DiscoverResponse = (MyDiscover*)buffer;
	DiscoverResponse->Revision = MY_DISCOVER_HEADER_VERSION;
	DiscoverResponse->MY_HWID = MY_DISCOVER_HARDWARE_ID;
	
	if (page==MY_DISCOVER_PAGEID_PARENT) {
		DiscoverResponse->PAGE_ID = MY_DISCOVER_PAGEID_PARENT;
		DiscoverResponse->PAGE_TYPE_ID = MY_DISCOVER_TYPEID_PARENT;
		DiscoverResponse->data[0] = _nc.parentNodeId; 
		*len = 3;
	} else if (page==MY_DISCOVER_PAGEID_GENERAL) {
		DiscoverResponse->PAGE_ID = MY_DISCOVER_PAGEID_GENERAL;
		DiscoverResponse->PAGE_TYPE_ID = MY_DISCOVER_TYPEID_GENERAL;
		DiscoverResponse->data[0] = (MY_TRANSPORT_COUNT << 5) | MY_DISCOVER_TOTAL_PAGES; 
		DiscoverResponse->data[1] = MY_DISCOVER_NODE_TYPE; 
		DiscoverResponse->data[2] = _nc.parentNodeId; 
		DiscoverResponse->data[3] = _nc.distance; 
		DiscoverResponse->data[4] = MY_DISCOVER_NODE_FEATURES; 
		memcpy(&(DiscoverResponse->data[5]),&_heartbeat,4);

		NodeFirmwareConfig _discover_fw;
		hwReadConfigBlock((void*)&_discover_fw, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(NodeFirmwareConfig));
		memcpy(&(DiscoverResponse->data[9]),&_discover_fw.crc,2); 
		memcpy(&(DiscoverResponse->data[11]),&_discover_fw.version,2); 
		memcpy(&(DiscoverResponse->data[13]),&_discover_fw.type,2); 
		memcpy(&(DiscoverResponse->data[15]),&_discover_fw.blocks,2); 	
		*len = 17;
	} else if (page==MY_DISCOVER_PAGEID_ARCHITECTURE) {
		DiscoverResponse->PAGE_ID = MY_DISCOVER_PAGEID_ARCHITECTURE;
		DiscoverResponse->PAGE_TYPE_ID = MY_DISCOVER_TYPEID_ARCHITECTURE;
		#if defined(ARDUINO_ARCH_AVR)
			DiscoverResponse->data[0] = boot_signature_byte_get(0x00);
			DiscoverResponse->data[1] = boot_signature_byte_get(0x02);
			DiscoverResponse->data[2] = boot_signature_byte_get(0x04);
			DiscoverResponse->data[3] = 0xFF; //MCUSR, WIP
			uint16_t cpu_voltage = hwCPUVoltage();
			memcpy(&(DiscoverResponse->data[4]),&cpu_voltage,2);
			uint16_t cpu_frequency = hwCPUFrequency();
			memcpy(&(DiscoverResponse->data[6]),&cpu_frequency,2);
			DiscoverResponse->data[8] = boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS);
			DiscoverResponse->data[9] = boot_lock_fuse_bits_get(GET_LOCK_BITS);
			DiscoverResponse->data[10] = boot_lock_fuse_bits_get(GET_EXTENDED_FUSE_BITS);
			DiscoverResponse->data[11]= boot_lock_fuse_bits_get(GET_HIGH_FUSE_BITS);
			
			uint16_t cpu_freemem = hwFreeMem();
			memcpy(&(DiscoverResponse->data[12]),&cpu_freemem,2);
			*len = 16;
		#elif defined(ARDUINO_ARCH_ESP8266)
			uint32_t cpu_id = ESP.getChipId();
			memcpy(&(DiscoverResponse->data[0]),&cpu_id,4);
			uint32_t flash_id = ESP.getFlashChipId();
			memcpy(&(DiscoverResponse->data[4]),&flash_id,4);
			uint16_t cpu_voltage = hwCPUVoltage();
			memcpy(&(DiscoverResponse->data[8]),&cpu_voltage,2);
			uint16_t cpu_frequency = hwCPUFrequency();
			memcpy(&(DiscoverResponse->data[10]),&cpu_frequency,2);
			uint16_t cpu_freemem = hwFreeMem();
			memcpy(&(DiscoverResponse->data[12]),&cpu_freemem,2);
			*len = 16;
		#elif defined(ARDUINO_ARCH_SAMD)
			// WIP
			*len = 2;
		#endif
	} else if (page==MY_DISCOVER_PAGEID_BOOTLOADER) {
		DiscoverResponse->PAGE_ID = MY_DISCOVER_PAGEID_BOOTLOADER;
		DiscoverResponse->PAGE_TYPE_ID = MY_DISCOVER_TYPEID_BOOTLOADER;
		*len = 2;
	} else if (page==MY_DISCOVER_PAGEID_TRANSPORT_UPLINK) {
		DiscoverResponse->PAGE_ID = MY_DISCOVER_PAGEID_TRANSPORT_UPLINK;
		DiscoverResponse->PAGE_TYPE_ID = MY_DISCOVER_TYPEID_TRANSPORT;
		DiscoverResponse->data[0] = MY_DISCOVER_TRANSPORT_PARENT;
		DiscoverResponse->data[1] = MY_DISCOVER_SIGNATURES;
		DiscoverResponse->data[2] = MY_DISCOVER_ENCRYPTION;
		DiscoverResponse->data[3] = MY_DISCOVER_PARENT_DATA_RATE;
		DiscoverResponse->data[4] = MY_DISCOVER_PARENT_POWER_LEVEL;
		DiscoverResponse->data[5] = MY_DISCOVER_PARENT_CHANNEL >> 8;
		DiscoverResponse->data[6] = MY_DISCOVER_PARENT_CHANNEL & 0xFF;
		#if defined(MY_GATEWAY_W5100) || defined(MY_GATEWAY_ENC28J60) || defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_MQTT_CLIENT)
			#if defined(MY_IP_ADDRESS)
				IPAddress _discover_IP(MY_IP_ADDRESS);
				memcpy(&(DiscoverResponse->data[7]),&(_discover_IP[0]),4);
				*len = 13;
			#else
				// DHCP option, WIP
				memset(&(DiscoverResponse->data[7]),0xFF,4);
				*len = 13;
			#endif 
		#elif defined(MY_RADIO_NRF24)
			DiscoverResponse->data[7] = (MY_RF24_BASE_RADIO_ID >> 32) & 0xFF;
			DiscoverResponse->data[8] = (MY_RF24_BASE_RADIO_ID >> 24) & 0xFF;
			DiscoverResponse->data[9] = (MY_RF24_BASE_RADIO_ID >> 16) & 0xFF;
			DiscoverResponse->data[10] = (MY_RF24_BASE_RADIO_ID >> 8) & 0xFF;
			*len = 13;
		#elif defined(MY_RADIO_RFM69)
			DiscoverResponse->data[7] = MY_RFM69_NETWORKID;
			*len = 10;
		#else
			*len = 9;
		#endif
	}
	
}

