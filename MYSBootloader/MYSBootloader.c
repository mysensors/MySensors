/*
 MYSBootloader: OTA bootloader for Mysensor nodes (www.mysensors.org)
 Code based on OTA bootloader by ToSa
 Optimized and extended by tekka
 Version 1.0 / 20150206
 Size: 1834 bytes
 
 Tested with Atmega328P, 16Mhz 3.3V and 5V and MYSController 0.1.2.266 (goo.gl/9DCWNo)
 
 MCU: atmega328
 clock: 16Mhz (prescaler: off/1)
 bootsz: 1024W
 

 fuses for ISP:
 EX = 0xFE (use 0x06 for Arduino IDE, boards.txt)
 HI = 0xDA
 LO = 0xF7
 
 nRF24L01+ connected to pins:
 CE = 9
 CSN = 10
 
 Successfully tested with:
 
 16Mhz extXTAL, 3.3V
 1Mhz intRC (8Mhz + DIV8), 3.3V
 128kHz intRC, 3.3V
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "MYSBootloader.h"


#define MYSBOOTLOADER_MAJVER 1
#define MYSBOOTLOADER_MINVER 0

#define MYSBOOTLOADER_VERSION (MYSBOOTLOADER_MINVER * 256 + MYSBOOTLOADER_MAJVER)

//#define USE_PRESCALER
#define MAX_RESEND 5


// procedures and functions

int main(void) __attribute__ ((OS_main)) __attribute__ ((section (".init9")));
static uint16_t calcCRCrom (const void* ptr, uint16_t len);
static uint8_t IsFirmwareValid();
static void reboot();
static void startup();
static void programPage(uint32_t page, uint8_t *buf);
static boolean sendWrite(MyMessage message);
static bool sendAndWait(uint8_t reqType, uint8_t resType);


static uint16_t calcCRCrom (const void* ptr, uint16_t len) {
	// init 0xFFFF
	uint16_t crc = ~0;
	for (uint16_t i = 0; i < len; ++i) {
		crc = _crc16_update(crc, pgm_read_byte((uint16_t) ptr + i));
	}
	return crc;
}

static uint8_t IsFirmwareValid () {
	return calcCRCrom(0, fc.blocks * FIRMWARE_BLOCK_SIZE) == fc.crc;
}

static void reboot() {
	// any pending eeprom activities?
	eeprom_busy_wait();
	// trigger watchdog ASAP
	watchdogConfig(WATCHDOG_16MS);
	// wait until WD triggers
	while (1);
}

static void startup() {
	if (IsFirmwareValid()) {
		// WD off
		watchdogConfig(WATCHDOG_OFF);
		
		#ifdef USE_PRESCALER
		// reset prescaler
		clock_prescale_set(orgClockDiv);
		#endif
		
		// start sketch
		((void(*)()) 0)();
				
	} else {
		reboot();
	}
}

static void programPage(uint32_t page, uint8_t *buf) {
	// using out commands, saves some bytes :)
	__boot_page_erase_short(page);
	boot_spm_busy_wait();	
	for (uint16_t i = 0; i < SPM_PAGESIZE; i += 2) {
		uint16_t w = *buf++;
		// generate word
		w += (*buf++) << 8;
		__boot_page_fill_short(page + i, w);
	}
	__boot_page_write_short(page);
	boot_spm_busy_wait();
	__boot_rww_enable_short();
}


static boolean sendWrite(MyMessage message) {
	return write(nc.parentNodeId, message.array, HEADER_SIZE + mGetLength(message), (message.destination == BROADCAST_ADDRESS));
}

static bool sendAndWait(uint8_t reqType, uint8_t resType) {
	outMsg.type = reqType;
	// outer loop, retries
	for (uint8_t i = 0; i < MAX_RESEND; i++) {
		sendWrite(outMsg);
		// loop 20 times, wait time 0.1ms if no/wrong data => 2s
		for (uint8_t j = 0; j < 20; j++) {
			// loop 100 times, wait 1ms if no/wrong data => 0.1s
			for (uint8_t k = 0; k < 100; k++) {
				watchdogReset();
				// Tx FIFO data available? (we don't care about the pipe here)
				if (available(NULL)) {
					// read message from FIFO
					readMessage(inMsg.array);
					// protocol compatible? if not ignore msg
					if ((mGetVersion(inMsg) != PROTOCOL_VERSION)) {
						continue;
					}
					if (inMsg.destination == nc.nodeId) {
						// msg for us
						if ((mGetCommand(inMsg) == C_INTERNAL) && (inMsg.type == I_FIND_PARENT_RESPONSE)) {
							if (inMsg.bValue < nc.distance - 1) {
								// got new routing info, update settings
								nc.distance = inMsg.bValue + 1;
								nc.parentNodeId = inMsg.sender;
							}
						}
						// did we receive expected reply?
						if ((mGetCommand(inMsg) == mGetCommand(outMsg)) && (inMsg.type == resType)) {
							return true;
						}
						
					}
				} else {
					// wait 1ms if no data available
					delaym(10);
				}
			}
		}
	}
	return false;
}




// main start
int main(void) {	
	
	asm volatile ("clr __zero_reg__");
	// reset MCU status register	
	MCUSR = 0;
	
	#ifdef USE_PRESCALER
	// get current prescale
	orgClockDiv = clock_prescale_get();
	//switch to 4 MHz on 16Mhz crystal
	clock_prescale_set(F_CPU_DIV);
	#endif	
		
	// enable watchdog to avoid deadlock
	watchdogConfig(WATCHDOG_8S);

	// initialize SPI
	SPIinit();
		
	// initialize RF module	
	RFinit();

	// Read node config from EEPROM, i.e. nodeId, parent nodeId, distance
	eeprom_read_block((void*)&nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(struct NodeConfig));
	// Read firmware config from EEPROM, i.e. type, version, CRC, blocks
	eeprom_read_block((void*)&fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(NodeFirmwareConfig));
	
	// bootloader should find closest node during each reboot

	// invalidate parent node settings, since we have to re-discover them for every single reboot
	nc.distance = 0xFF;
	nc.parentNodeId = 0xFF;
	
	// prepare for I_FIND_PARENTS
	outMsg.sender = nc.nodeId;
	outMsg.last = nc.nodeId;
	outMsg.sensor = 0xFF;
	outMsg.destination = BROADCAST_ADDRESS;
	
	mSetVersion(outMsg, PROTOCOL_VERSION);
	mSetLength(outMsg, 0);
	mSetCommand(outMsg, C_INTERNAL);
	mSetAck(outMsg,false);
	mSetPayloadType(outMsg, P_STRING);
	
	// set reading & writing pipe address
	setAddress(nc.nodeId);

	// network up?, get neighbors, else startup
	if (!sendAndWait(I_FIND_PARENT, I_FIND_PARENT_RESPONSE)) {
		startup();
	}
	
	// all messages to gateway
	outMsg.destination = GATEWAY_ADDRESS;
	
	// if no node id assigned, request new id
	if (nc.nodeId == AUTO) {
		// listen to broadcast
		openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(BROADCAST_ADDRESS));
		if (sendAndWait(I_ID_REQUEST, I_ID_RESPONSE)) {
			// save id to eeprom
			eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, atoi(inMsg.data));
		}
		// we could go on and set everything right here, but rebooting will take care of that - saves bytes :)
		reboot();				
	}
	
	// wuff
	watchdogReset();
	// prepare for FW config request
	RequestFirmwareConfig *reqFWConfig = (RequestFirmwareConfig *)outMsg.data;	
	mSetLength(outMsg, sizeof(RequestFirmwareConfig));
	mSetCommand(outMsg, C_STREAM);
	mSetPayloadType(outMsg,P_CUSTOM);
	// copy node settings to reqFWConfig
	memcpy(reqFWConfig,&fc,sizeof(NodeFirmwareConfig));
	// add bootloader information
	reqFWConfig->BLVersion = MYSBOOTLOADER_VERSION;
	
	// send node config and request FW config from controller
	if (!sendAndWait(ST_FIRMWARE_CONFIG_REQUEST, ST_FIRMWARE_CONFIG_RESPONSE)) {
		startup();
	}
	
	NodeFirmwareConfig *firmwareConfigResponse = (NodeFirmwareConfig *)inMsg.data;
	
	// compare with current node configuration, if equal startup
	if (!memcmp(&fc,firmwareConfigResponse,sizeof(NodeFirmwareConfig))) {
		startup();
	}
	
	// *********** from here on we will fetch new FW
	
	// invalidate current CRC
	fc.crc = 0xFFFF;
	// write fetched type and version in case OTA fails (BL will reboot and re-request FW with stored settings)
	eeprom_write_block(&fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS,sizeof(NodeFirmwareConfig));
	
	// copy new FW config
	memcpy(&fc,firmwareConfigResponse,sizeof(NodeFirmwareConfig));
	RequestFWBlock *firmwareRequest = (RequestFWBlock *)outMsg.data;
	mSetLength(outMsg, sizeof(RequestFWBlock));
	
	firmwareRequest->type = fc.type;
	firmwareRequest->version = fc.version;
	
	// request FW from controller
	for (uint16_t block = fc.blocks; block > 0; block--) {
		firmwareRequest->block = (block - 1);
		// request FW block
		if (!sendAndWait(ST_FIRMWARE_REQUEST, ST_FIRMWARE_RESPONSE)) {
			reboot();
		}
		
		ReplyFWBlock *firmwareResponse = (ReplyFWBlock *)inMsg.data;
		// calculate page offset
		uint8_t offset = ((block - 1) * FIRMWARE_BLOCK_SIZE) % SPM_PAGESIZE;
		// write to buffer
		memcpy(progBuf + offset, firmwareResponse->data, FIRMWARE_BLOCK_SIZE);
		// program if page full
		if (offset == 0) {
			programPage(((block - 1) * FIRMWARE_BLOCK_SIZE), progBuf);
		}
	}

	// wuff
	watchdogReset();
	
	// all blocks transmitted, calc CRC and write to eeprom if valid	
	if (IsFirmwareValid()) {
		// if FW is valid, write settings to eeprom 
		eeprom_write_block((void*)&fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(NodeFirmwareConfig));
		// we could restart directly here, but eeprom may still be busy, thus reboot (we check for idle eeprom before rebooting) and save some bytes :)
	} 
	// final step
	reboot();
}




