#include "MyOtaBootloader.h"

int main(void) __attribute__ ((naked)) __attribute__ ((section (".init9")));

volatile char dummy;

EMPTY_INTERRUPT(WDT_vect);

static uint16_t calcCRCrom (const void* ptr, uint16_t len) {
	uint16_t crc = ~0;
	for (uint16_t i = 0; i < len; ++i)
		crc = _crc16_update(crc, pgm_read_byte((uint16_t) ptr + i));
	return crc;
}

static uint8_t validFirmware () {
	return calcCRCrom(0, fc.blocks * FIRMWARE_BLOCK_SIZE) == fc.crc;
}

static void reboot() {
	wdt_enable(WDTO_15MS);
	for (;;);
}

static void startup() {
	if (validFirmware()) {
		wdt_disable();
		clock_prescale_set(orgClockDiv);
		((void(*)()) 0)();
	} else
		reboot();
}

static void boot_program_page(uint32_t page, uint8_t *buf) {
	uint8_t sreg = SREG;
	cli();
	eeprom_busy_wait();
	boot_page_erase(page);
	boot_spm_busy_wait();
	for (uint16_t i = 0; i < SPM_PAGESIZE; i += 2) {
		uint16_t w = *buf++;
		w += (*buf++) << 8;
		boot_page_fill(page + i, w);
	}
	boot_page_write(page);
	boot_spm_busy_wait();
	boot_rww_enable();
	SREG = sreg;
}

static boolean sendWrite(MyMessage message) {
	return write(nc.parentNodeId, message.array, HEADER_SIZE + mGetLength(message), (message.destination == BROADCAST_ADDRESS));
}

static uint8_t sendAndWait(uint8_t reqType, uint8_t resType) {
	msg.type = reqType;
	for (uint8_t i = 0; i < 10; i++) {
		sendWrite(msg);
		for (uint8_t j = 0; j < 20; j++) {
			wdt_reset();
			for (uint8_t j = 0; j < 100; j++) {
				wdt_reset();
				uint8_t pipe;
				boolean avail = available(&pipe);
				if (avail && pipe<=6) {
					read(rmsg.array,pipe);
					if(!(mGetVersion(rmsg) == PROTOCOL_VERSION))
						continue;
					if (rmsg.destination == nc.nodeId) {
						if (mGetCommand(rmsg) == C_INTERNAL) {
							if (rmsg.type == I_FIND_PARENT_RESPONSE) {
								if (rmsg.data[0] < nc.distance - 1) {
									nc.distance = rmsg.data[0] + 1;
									nc.parentNodeId = rmsg.sender;
									eeprom_write_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, nc.parentNodeId);
									eeprom_write_byte((uint8_t*)EEPROM_DISTANCE_ADDRESS, nc.distance);
								}
							}
						}
						if ((mGetCommand(rmsg) == mGetCommand(msg)) && (rmsg.type == resType))
							return 1;
					}
				}
				delaym(1);
			}
		}
	}
	return 0;
}

int main () {
	asm volatile ("clr __zero_reg__");
	// switch to 4 MHz
	orgClockDiv = clock_prescale_get();
	clock_prescale_set(F_CPU_DIV);
	MCUSR = 0;
	// enable watchdog to avoid deadlock
	wdt_enable(WDTO_8S);

#ifdef LED_DEBUG
	// enable LEDs for debugging and blink once as visual "bootloader started" sign
	led_init();
	blink(LED_MBOTH, 250);
#endif
#ifdef UART_DEBUG
	// enable UART for debugging and send start message
	uart_init();
	uart_puts("Go\n");
#endif

	wdt_reset();
	// initialize RF module
	begin();
	wdt_reset();
	// Read settings from EEPROM
	eeprom_read_block((void*)&nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(struct NodeConfig));
	wdt_reset();

	msg.sender = nc.nodeId;
	msg.last = nc.nodeId;
	address(nc.nodeId);

	msg.sensor = NODE_SENSOR_ID;
	mSetLength(msg, 0);
	mSetPayloadType(msg, P_STRING);
	mSetAck(msg,false);
	mSetVersion(msg, PROTOCOL_VERSION);

	nc.distance = 255;
	mSetCommand(msg, C_INTERNAL);
	msg.destination = BROADCAST_ADDRESS;
	if (!sendAndWait(I_FIND_PARENT, I_FIND_PARENT_RESPONSE))
//	starting existing firmware if no parent responds (if firmware is valid)
//		reboot();
		startup();
		
	msg.destination = GATEWAY_ADDRESS;

	if (nc.nodeId == AUTO) {
		openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));
		if (!sendAndWait(I_ID_REQUEST, I_ID_RESPONSE))
			reboot();
		if (mGetPayloadType(rmsg) == P_BYTE)
			nc.nodeId = rmsg.data[0];
		else if (mGetPayloadType(rmsg) == P_STRING)
			nc.nodeId = atoi(rmsg.data);
		if (nc.nodeId == AUTO)
			reboot();
		eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, nc.nodeId);
		msg.sender = nc.nodeId;
		msg.last = nc.nodeId;
		address(nc.nodeId);
	}
	
	// Inform gateway that bootloader does not accept signed messages
	msg.type = I_REQUEST_SIGNING;
	mSetLength(msg, 1);
	msg.data[0] = 0;
	sendWrite(msg);

	// Read settings from EEPROM
	eeprom_read_block((void*)&fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(struct FirmwareConfig));

	mSetPayloadType(msg,P_CUSTOM);

	FirmwareConfigRequest *firmwareConfigRequest = (FirmwareConfigRequest *)msg.data;
	mSetLength(msg, sizeof(FirmwareConfigRequest));
	mSetCommand(msg, C_STREAM);
	firmwareConfigRequest->type = fc.type;
	firmwareConfigRequest->version = fc.version;
	if (!sendAndWait(ST_FIRMWARE_CONFIG_REQUEST, ST_FIRMWARE_CONFIG_RESPONSE))
//	starting existing firmware if no firmware config response (if firmware is valid)
//		reboot();
		startup();
	FirmwareConfigResponse *firmwareConfigResponse = (FirmwareConfigResponse *)rmsg.data;
	if (firmwareConfigResponse->version == fc.version)
		if (firmwareConfigResponse->blocks == fc.blocks)
			if (firmwareConfigResponse->crc == fc.crc)
				if (validFirmware())
					startup();
	fc.type = firmwareConfigResponse->type;
	fc.version = firmwareConfigResponse->version;
	fc.blocks = firmwareConfigResponse->blocks;
	fc.crc = firmwareConfigResponse->crc;

	FirmwareRequest *firmwareRequest = (FirmwareRequest *)msg.data;
	mSetLength(msg, sizeof(FirmwareRequest));
	firmwareRequest->type = fc.type;
	firmwareRequest->version = fc.version;
	for (uint16_t block = 0; block < fc.blocks; block++) {
		firmwareRequest->block = block;
		if (!sendAndWait(ST_FIRMWARE_REQUEST, ST_FIRMWARE_RESPONSE))
			reboot();
		FirmwareResponse *firmwareResponse = (FirmwareResponse *)rmsg.data;
		uint8_t offset = (block * FIRMWARE_BLOCK_SIZE) % SPM_PAGESIZE;
		memcpy(progBuf + offset, firmwareResponse->data, FIRMWARE_BLOCK_SIZE);
		if (offset == SPM_PAGESIZE - FIRMWARE_BLOCK_SIZE)
			boot_program_page((block * FIRMWARE_BLOCK_SIZE) - offset, progBuf);
	}

	wdt_reset();
	
	if (validFirmware()) {
		eeprom_write_block((void*)&fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(struct FirmwareConfig));
#ifdef UART_DEBUG
		uart_puts("fw ok\n");
	} else {
		uart_puts("fw nok\n");
#endif
	}

	reboot();
}
