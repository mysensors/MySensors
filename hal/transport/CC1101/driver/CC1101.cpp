
#include "CC1101.h"
#include "Arduino.h"

// debug
#ifdef MY_DEBUG_VERBOSE_CC1101
#define MY_DEBUG_CC1101
#define CC1101_DEBUG_V(x, ...) DEBUG_OUTPUT(x, ##__VA_ARGS__) //!< Debug print
#else
#define CC1101_DEBUG_V(x, ...) //!< DEBUG null
#endif

#ifdef MY_DEBUG_CC1101
#define CC1101_DEBUG(x, ...) DEBUG_OUTPUT(x, ##__VA_ARGS__) //!< Debug print
#else
#define CC1101_DEBUG(x, ...) //!< DEBUG null
#endif

// Global status variable
static cc1101_internal_t CC1101;

// Convert from chip RSSI value to "real" RSSI.
static int8_t CC1101_RSSI_from_chip(const int8_t chipRSSI)
{
	return chipRSSI - CC1101_RSSI_OFFSET;
}

uint8_t PA_TABLE_315[8] {
	//  -30    -20  -15   -10     0     5    7    10
	0x12, 0x0D, 0x1C, 0x34, 0x51, 0x85, 0xCB, 0xC2,
};
uint8_t PA_TABLE_433[8] {
	//  -30    -20  -15   -10     0     5    7    10
	0x12, 0x0E, 0x1D, 0x34, 0x60, 0x84, 0xC8, 0xC0,
};
uint8_t PA_TABLE_868[8] {
	//  -30    -20  -15   -10     0     5    7    10
	0x03, 0x0F, 0x1E, 0x27, 0x50, 0x81, 0xCB, 0xC2,
};
uint8_t PA_TABLE_915[8] {
	//  -30    -20  -15   -10     0     5    7    10
	0x03, 0x0E, 0x1E, 0x27, 0x8E, 0xCD, 0xC7, 0xC0,
};

static bool CC1101_initialise()
{
	// setting pin modes
	CC1101_DEBUG(PSTR("CC1101:INIT\n"));
	hwPinMode(MY_CC1101_GD0_PIN, INPUT);
	CC1101_DEBUG(PSTR("CC1101:INIT:GD0PIN=%u\n"), MY_CC1101_GD0_PIN);

#if !defined(__linux__)
	hwDigitalWrite(MY_CC1101_CS_PIN, HIGH);
	hwPinMode(MY_CC1101_CS_PIN, OUTPUT);
#endif
	CC1101_SPI.begin();

	CC1101.address = CC1101_BROADCAST_ADDRESS;
	CC1101.ackReceived = false;
	CC1101.dataReceived = false;
	CC1101.txSequenceNumber = 0;
#if (MY_CC1101_POWER_LEVEL) == (CC1101_POWER_AUTO)
	CC1101.powerLevel = CC1101_POWER_7; // initial power.
	CC1101.ATCenabled = true;
#else
	CC1101.powerLevel = MY_CC1101_POWER_LEVEL; // fixed power.
	CC1101.ATCenabled = false;
#endif
	CC1101.targetRSSI = CC1101_TARGET_RSSI;
	CC1101.txComplete = true;

	// Hardware check - when CSN is pulled low, SO should go low.
	if (!CC1101_csnLow()) {
		CC1101_DEBUG(PSTR("!CC1101:INIT:SANCHK FAIL\n"));
		return false;
	}
	CC1101_csnHigh();

	// Reset and wait for chip status to indicate ready.
	CC1101_sendCommand(CC1101_CMD_SRES);
	do {
		CC1101_sendCommand(CC1101_CMD_SNOP);
	} while (CC1101.chipStatus & 0x80);

	CC1101_configure();
	attachInterrupt(MY_CC1101_GD0_NUM, CC1101_interruptHandler, FALLING);

	if (CC1101_sanityCheck()) {
		return true;
	}
	CC1101_DEBUG(PSTR("!CC1101:INIT:SANCHK FAIL\n"));
	return false;
}

static void CC1101_configure()
{
	CC1101_sendRegister(CC1101_REG_MDMCFG4, MY_CC1101_REG_MDMCFG4_VALUE);
	CC1101_sendRegister(CC1101_REG_MDMCFG3, MY_CC1101_REG_MDMCFG3_VALUE);
	CC1101_sendRegister(CC1101_REG_MDMCFG2, MY_CC1101_REG_MDMCFG2_VALUE);
	CC1101_sendRegister(CC1101_REG_DEVIATN, MY_CC1101_REG_DEVIATN_VALUE);
	// - Default 2-FSK modulation.
	// - Default disable manchester.
	// - Default 2 sync bytes.
	//  CC1101_sendRegister(CC1101_REG_MDMCFG2, 0b00110000);
	// - Only calibrate manually.
	CC1101_sendRegister(CC1101_REG_MCSM0, 0b00000100);
	// - Jump to IDLE after RX and TX complete.
	// - Enable CCA check on RX.
	CC1101_sendRegister(CC1101_REG_MCSM1, 0b00100000);

	// - Enable data whitening.
	// - CRC enabled.
	// - Variable packet length.
#ifdef MY_CC1101_DISABLE_WHITENING
	CC1101_sendRegister(CC1101_REG_PKTCTRL0, 0b00000101);
#else
	CC1101_sendRegister(CC1101_REG_PKTCTRL0, 0b01000101);
#endif

	// - Address check with broadcast.
	// - Append 2 status bytes.
	CC1101_sendRegister(CC1101_REG_PKTCTRL1, 0b00001111);

	// Set maximum allowed RX packet length.
	CC1101_sendRegister(CC1101_REG_PKTLEN, CC1101_MAX_PACKET_LEN);

	// - Set a non-default sync word.
	CC1101_sendRegister(CC1101_REG_SYNC0, MY_CC1101_SYNC_WORD >> 8);
	CC1101_sendRegister(CC1101_REG_SYNC1, MY_CC1101_SYNC_WORD & 0xFF);

	CC1101_setFrequency();
	CC1101_txPower(CC1101.powerLevel);

	// Calibrate and wait til done.
	CC1101_sendCommand(CC1101_CMD_SCAL);
	do {
		CC1101_sendCommand(CC1101_CMD_SNOP);
	} while (CC1101.chipStatus & 0x40);

	// Set GD0 to signal TX/RX completion.
	CC1101_sendRegister(CC1101_REG_IOCFG0, 0x06);
}

#ifdef ESP32
void IRAM_ATTR CC1101_interruptHandler()
#else
static void CC1101_interruptHandler()
#endif
{
#ifndef ESP32
	noInterrupts();
#endif
	CC1101.irqFired = true;
	CC1101.radioMode = CC1101_MODE_IDLE;
#ifndef ESP32
	interrupts();
#endif
}

static void CC1101_handle()
{
	if (CC1101.irqFired) {
		CC1101.irqFired = false;
		uint8_t rxBytesReg = CC1101_readRegister(CC1101_REG_RXBYTES);
		uint8_t txBytesReg = CC1101_readRegister(CC1101_REG_TXBYTES);
		uint8_t rxBytesLen = rxBytesReg & 0x7F;
		uint8_t txBytesLen = txBytesReg & 0x7F;
		CC1101_DEBUG_V(PSTR("CC1101:IRQ:RXLEN=%d(%d):TXLEN=%d(%d):CHST=0x%02x\n"),
		               rxBytesLen, rxBytesReg >> 7, txBytesLen, txBytesReg >> 7,
		               CC1101.chipStatus);
		// Transmission done
		if (!CC1101.txComplete && !txBytesLen) {
			CC1101.txComplete = true;
			//            CC1101_sendCommand(CC1101_CMD_SFTX);
			CC1101_rx();
		}

		// Reception done
		if (rxBytesLen) {
			if (!(rxBytesReg >> 7)) {
				CC1101.currentPacket.payloadLen = rxBytesLen - CC1101_HEADER_LEN - 2;
				CC1101_readFifo(CC1101.currentPacket.data, rxBytesLen);
				uint8_t chipRSSI = CC1101.currentPacket.data[rxBytesLen - 2];
				CC1101.currentPacket.RSSI = CC1101_RSSI_from_chip((int8_t)chipRSSI);
				CC1101.currentPacket.LQI =
				    CC1101.currentPacket.data[rxBytesLen - 1] & 0x7F;
#ifdef MY_DEBUG_CC1101
				hwDebugBuf2Str((const uint8_t *)CC1101.currentPacket.data, rxBytesLen);
				CC1101_DEBUG(PSTR("CC1101:RECV:DATA=%s,RSSI=%d LQI=%d\n"),
				             hwDebugPrintStr, CC1101.currentPacket.RSSI, CC1101.currentPacket.LQI);
#endif
				if ((CC1101.currentPacket.header.version >=
				        CC1101_MIN_PACKET_HEADER_VERSION) &&
				        ((CC1101.currentPacket.header.recipient != 0) || (CC1101.address == 0))) {
					CC1101.ackReceived =
					    CC1101.currentPacket.header.controlFlags.fields.ackReceived &&
					    !CC1101.currentPacket.header.controlFlags.fields.ackRequested;
					CC1101.dataReceived = !CC1101.ackReceived;
					CC1101.currentPacket.header
					.length++; // TX len is short by one for radio.
				}
			} else {
				CC1101_DEBUG(PSTR("CC1101:RECV:OVERFLOW\n"));
			}

			CC1101_sendCommand(CC1101_CMD_SFRX);
			CC1101_rx();
		}
	}
}

static bool CC1101_csnLow()
{
	hwDigitalWrite(MY_CC1101_CS_PIN, LOW);
	for (unsigned long start = hwMillis();
	        hwMillis() - start < MY_CC1101_CSN_TIMEOUT_MS;) {
		if (!hwDigitalRead(MISO)) {
			return true;    // SO went low as expected.
		}
	}
	// Did not get SO low in timeout. Wiring problem?
	CC1101_DEBUG(PSTR("!CC1101:CSN:TMOUT=%d\n"), MY_CC1101_CSN_TIMEOUT_MS);
	return false;
}

static void CC1101_csnHigh()
{
	hwDigitalWrite(MY_CC1101_CS_PIN, HIGH);
}

static void CC1101_sendCommand(cc1101_commands_t command)
{
	CC1101_csnLow();
	CC1101.chipStatus = CC1101_SPI.transfer(command | CC1101_WRITE_SINGLE);
	CC1101_csnHigh();
	CC1101_DEBUG_V(PSTR("CC1101:CMDSTR=0x%x:STATUS=0x%02x\n"), command,
	               CC1101.chipStatus);
}

uint8_t CC1101_readRegister(cc1101_registers_t address)
{
	CC1101_csnLow();
	CC1101.chipStatus = CC1101_SPI.transfer(address | CC1101_READ_SINGLE);
	uint8_t val = CC1101_SPI.transfer(0x0);
	CC1101_csnHigh();
	CC1101_DEBUG_V(PSTR("CC1101:RREG:0x%x=0x%x\n"), address, val);
	return val;
}

void CC1101_sendRegister(cc1101_registers_t reg, uint8_t value)
{
	CC1101_DEBUG_V(PSTR("CC1101:WREG:0x%x=0x%x\n"), reg, value);
	CC1101_csnLow();
	CC1101.chipStatus = CC1101_SPI.transfer(reg | CC1101_WRITE_SINGLE);
	CC1101_SPI.transfer(value);
	CC1101_csnHigh();
}

static void CC1101_sendRegisterBurst(cc1101_registers_t reg,
                                     const uint8_t *buffer, uint8_t size)
{
	CC1101_DEBUG_V(PSTR("CC1101:WREGB:0x%x:LEN=%d\n"), reg, size);
	CC1101_csnLow();
	CC1101_SPI.transfer(reg | CC1101_WRITE_BURST);
	for (uint8_t i = 0; i < size; i++) {
		CC1101_SPI.transfer(buffer[i]);
	}
	CC1101_csnHigh();
}

static void CC1101_readRegisterBurst(cc1101_registers_t reg, uint8_t *buffer,
                                     uint8_t size)
{
	CC1101_csnLow();
	CC1101_SPI.transfer(reg | CC1101_READ_BURST);
	for (uint8_t i = 0; i < size; i++) {
		buffer[i] = CC1101_SPI.transfer(0x00);
	}
	CC1101_csnHigh();
}

static void CC1101_sendFifo(const uint8_t *buffer, const uint8_t size)
{
	CC1101_DEBUG_V(PSTR("CC1101:SENDFIFO:LEN=%d\n"), size);
	CC1101_sendRegisterBurst(CC1101_REG_FIFO, buffer, size);
}

static void CC1101_readFifo(uint8_t *buffer, const uint8_t size)
{
	CC1101_readRegisterBurst(CC1101_REG_FIFO, buffer, size);
}

static bool CC1101_sanityCheck()
{
	if (!CC1101_csnLow()) {
		CC1101_DEBUG(PSTR("!CC1101:INIT:SANCHK FAIL\n"));
		return false;
	}
	CC1101_csnHigh();
	uint8_t val = CC1101_readRegister(CC1101_REG_VERSION);
	if ((val != 0x14) && (val != 0x04)) {
		CC1101_DEBUG(PSTR("!CC1101:INIT:SANCHK FAIL:0x%02X\n"), val);
		return false;
	}
	return true;
}

static void CC1101_wakeUp()
{
	CC1101_csnLow();
	CC1101_csnHigh();
	CC1101.radioMode = CC1101_MODE_IDLE;
	CC1101_txPower(
	    CC1101.powerLevel); // PATABLE cleared on sleep. [Datasheet section 24]
	CC1101_rx();
}

static void CC1101_idle()
{
	CC1101_sendCommand(CC1101_CMD_SIDLE);
	CC1101.radioMode = CC1101_MODE_IDLE;
}

static void CC1101_sleep(void)
{
	// TODO: GDO might be forced low on sleep? Datasheet sec 26.
	CC1101_idle();
	CC1101_sendCommand(CC1101_CMD_SPWD);
	CC1101.radioMode = CC1101_MODE_SLEEP;
}

static void CC1101_tx()
{
	CC1101.txComplete = false;
	CC1101_sendCommand(CC1101_CMD_STX);
	CC1101.radioMode = CC1101_MODE_TX;
}

static void CC1101_rx()
{
	CC1101_sendCommand(CC1101_CMD_SRX);
	CC1101.radioMode = CC1101_MODE_RX;
}

static bool CC1101_sendWithRetry(const uint8_t recipient, const void *buffer,
                                 const uint8_t bufferSize, const bool noACK)
{
	cc1101_controlFlags_t flags{0};
	flags.fields.ackRequested = !noACK;
	CC1101.txSequenceNumber++;
	for (uint8_t retry = 0; retry < CC1101_SEND_RETRIES; retry++) {
		CC1101_DEBUG(
		    PSTR("CC1101:SWR:SEND,TO=%u,LEN=%d,SEQ=%u,RETRY=%u,NOACK=%d\n"),
		    recipient, bufferSize, CC1101.txSequenceNumber, retry, noACK);
		if (!CC1101_send(recipient, (uint8_t *)buffer, bufferSize, flags)) {
			return false;
		}
		if (noACK) {
			return true;
		}
		uint32_t start = hwMillis();
		while (hwMillis() < start + MY_CC1101_SEND_TIMEOUT_MS) {
			CC1101_handle();
			if (CC1101.ackReceived) {
				CC1101.ackReceived = false;
				// is it the ACK for our current packet?
				if (CC1101.currentPacket.header.sender == recipient &&
				        CC1101.currentPacket.ACK.sequenceNumber ==
				        CC1101.txSequenceNumber) {
					CC1101_DEBUG(PSTR("CC1101:SWR:ACK FROM=%u,SEQ=%u,RSSI=%d,LQI:%d\n"),
					             CC1101.currentPacket.header.sender,
					             CC1101.currentPacket.ACK.sequenceNumber,
					             CC1101.currentPacket.ACK.RSSI,
					             CC1101.currentPacket.ACK.LQI);
					CC1101.lastAck.ACK.LQI = CC1101.currentPacket.ACK.LQI;
					CC1101.lastAck.ACK.RSSI = CC1101.currentPacket.ACK.RSSI;
					if (CC1101.ATCenabled) {
						CC1101_ATC();
					}
					return true;
				}
			}
			doYield();
		}
		CC1101_DEBUG(PSTR("!CC1101:SWR:NACK\n"));
		const uint32_t enterCSMAMS = hwMillis();
		const uint16_t randDelayCSMA = start % 100;
		while (hwMillis() - enterCSMAMS < randDelayCSMA) {
			doYield();
		}
		if (CC1101.ATCenabled) {
			CC1101_txPower((cc1101_powerLevel_t)(
			                   CC1101.powerLevel +
			                   1)); // increase power, maybe we are far away from gateway
		}
	}
	return false;
}

static bool CC1101_send(const uint8_t recipient, uint8_t *data,
                        const uint8_t len, const cc1101_controlFlags_t flags)
{
	cc1101_packet_t packet;
	packet.header.version = CC1101_PACKET_HEADER_VERSION;
	packet.header.sender = CC1101.address;
	packet.header.recipient = recipient;
	packet.header.controlFlags = flags;
	packet.header.sequenceNumber = CC1101.txSequenceNumber;
	packet.payloadLen = min(len, (uint8_t)CC1101_MAX_PAYLOAD_LEN);
	packet.header.length = packet.payloadLen + CC1101_HEADER_LEN - 1;
	memcpy((void *)&packet.payload, (void *)data, packet.payloadLen);
	return CC1101_sendPacket(&packet);
}

static bool CC1101_sendPacket(cc1101_packet_t *packet)
{
	uint32_t txStart = hwMillis();
	CC1101.txComplete = false;
	for (uint8_t i = 0; i < CC1101_TX_RETRIES && !CC1101.txComplete; i++) {
		CC1101_sendFifo(packet->data, packet->header.length + 1);
		CC1101_tx();
		uint32_t txTimeoutStart = hwMillis();

		while (!CC1101.txComplete) {
			CC1101_handle();
			if (hwMillis() > txTimeoutStart + MY_CC1101_TX_TIMEOUT_MS) {
				uint8_t txBytesReg = CC1101_readRegister(CC1101_REG_TXBYTES);
				if ((txBytesReg & 0x7F) && ((CC1101.chipStatus & 0x70) != 0x20)) {
					CC1101_DEBUG(PSTR("!CC1101:CCA:FAIL\n"));
					CC1101_idle();
					CC1101_sendCommand(CC1101_CMD_SFTX);
				} else {
					CC1101_DEBUG(PSTR("!CC1101:TXTMOUT\n"));
				}
				CC1101_rx();
				delay(MY_CC1101_TX_RETRY_DELAY_MS);
				break;
			}
			doYield();
		}
	}
	CC1101_DEBUG(PSTR("CC1101:TXCMPL:MILLIS=%d\n"), hwMillis() - txStart);
	return true;
}

static bool CC1101_packetAvailable()
{
	if (CC1101.radioMode != CC1101_MODE_RX &&
	        CC1101.radioMode != CC1101_MODE_TX) {
		// if we are not sending or already in receive, go into receive;
		CC1101_rx();
	}
	return CC1101.dataReceived;
}

static uint8_t CC1101_getData(uint8_t *buffer, const uint8_t bufferSize)
{
	const uint8_t payloadSize = min(CC1101.currentPacket.payloadLen, bufferSize);
	if (buffer != NULL) {
		(void)memcpy((void *)buffer, (void *)&CC1101.currentPacket.payload,
		             payloadSize);
	}
	// clear data flag
	CC1101.dataReceived = false;
	// ACK handling
	if (CC1101.currentPacket.header.controlFlags.fields.ackRequested &&
	        !CC1101.currentPacket.header.controlFlags.fields.ackReceived) {
		delay(MY_CC1101_ACK_SEND_DELAY_MS);
		CC1101_sendAck(CC1101.currentPacket.header.sender,
		               CC1101.currentPacket.header.sequenceNumber,
		               CC1101.currentPacket.RSSI, CC1101.currentPacket.LQI);
	}
	return payloadSize;
}

static void CC1101_sendAck(const uint8_t recipient,
                           const cc1101_sequenceNumber_t sequenceNumber,
                           const cc1101_RSSI_t RSSI, const cc1101_LQI_t LQI)
{
	CC1101_DEBUG(PSTR("CC1101:SAC:SEND ACK,TO=%u,SEQ=%u,RSSI=%d,LQI=%u\n"),
	             recipient, sequenceNumber, RSSI, LQI);
	cc1101_ack_t ACK;
	ACK.sequenceNumber = sequenceNumber;
	ACK.RSSI = RSSI;
	ACK.LQI = LQI;
	cc1101_controlFlags_t flags = {0};
	flags.fields.ackReceived = true;
	flags.fields.ackRssiReport = true;
	(void)CC1101_send(recipient, (uint8_t *)&ACK, sizeof(cc1101_ack_t), flags);
}

static void CC1101_txPower(cc1101_powerLevel_t power)
{
	if (power > MY_CC1101_MAX_POWER_LEVEL) {
		return;
	}
	CC1101_sendRegisterBurst(CC1101_REG_PATABLE, CC1101_PA_TABLE, 8);
	CC1101_sendRegister(CC1101_REG_FREND0, power | 0x10);
	CC1101.powerLevel = power;
	CC1101_DEBUG(PSTR("CC1101:PTC:LEVEL=%d\n"), CC1101.powerLevel);
}
static void CC1101_setFrequency()
{
	uint8_t freq[3] = {CC1101_FREQ_BYTE0, CC1101_FREQ_BYTE1, CC1101_FREQ_BYTE2};
	CC1101_sendRegisterBurst(CC1101_REG_FREQ2, freq,
	                         sizeof(freq) / sizeof(uint8_t));
	CC1101_sendRegister(CC1101_REG_CHANNR, MY_CC1101_CHANNEL);
	CC1101_DEBUG(PSTR("CC1101:FREQ:0x%02x%02x%02x\n"), freq[0], freq[1], freq[2]);
}

static void CC1101_setAddress(uint8_t address)
{
	CC1101.address = address;
	CC1101_sendRegister(CC1101_REG_ADDR, address);
}

static uint8_t CC1101_getAddress(void)
{
	CC1101.address = CC1101_readRegister(CC1101_REG_ADDR);
	return CC1101.address;
}

static void CC1101_ATC()
{
	int8_t delta = CC1101.currentPacket.ACK.RSSI - CC1101.targetRSSI;
	cc1101_powerLevel_t newPowerLevel = CC1101.powerLevel;

	if (delta > 0 && delta > CC1101_ATC_TARGET_RANGE_DBM) {
		// RSSI is too high, decrease power
		if (CC1101.powerLevel > MY_CC1101_MIN_POWER_LEVEL) {
			newPowerLevel = (cc1101_powerLevel_t)(CC1101.powerLevel - 1);
		}
	} else if (delta < 0 && delta < -CC1101_ATC_TARGET_RANGE_DBM) {
		// RSSI is too low, increase power
		if (CC1101.powerLevel < MY_CC1101_MAX_POWER_LEVEL) {
			newPowerLevel = (cc1101_powerLevel_t)(CC1101.powerLevel + 1);
		}
	}

	CC1101_DEBUG(PSTR("CC1101:ATC:cR=%d, tR=%d, pL=%d, newPL=%d\n"),
	             CC1101.currentPacket.ACK.RSSI, CC1101.targetRSSI,
	             CC1101.powerLevel, newPowerLevel);

	if (newPowerLevel != CC1101.powerLevel) {
		CC1101_txPower(newPowerLevel);
	}
}

static void CC1101_setATC(bool onOff, int8_t targetRSSI)
{
	CC1101.ATCenabled = onOff;
	CC1101.targetRSSI = targetRSSI;
}

static int16_t CC1101_getSendingRSSI(void)
{
	// own RSSI, as measured by the recipient - ACK part
	if (CC1101.currentPacket.header.controlFlags.fields.ackRssiReport) {
		return CC1101.currentPacket.ACK.RSSI;
	} else {
		// not possible
		return INVALID_RSSI;
	}
}

static int16_t CC1101_getReceivingRSSI(void)
{
	// RSSI from last received packet
	return static_cast<int16_t>(CC1101.currentPacket.RSSI);
}

static int8_t CC1101_getTxPowerLevel(void)
{
	return CC1101.powerLevel;
}

static uint8_t CC1101_getTxPowerPercent(void)
{
	// report TX level in %
	const uint8_t result = static_cast<uint8_t>(100.0f * CC1101.powerLevel / 8);
	return result;
}
static bool CC1101_setTxPowerPercent(const uint8_t newPowerPercent)
{
	const cc1101_powerLevel_t newPowerLevel = static_cast<cc1101_powerLevel_t>(
	            MY_CC1101_MIN_POWER_LEVEL +
	            (MY_CC1101_MAX_POWER_LEVEL - MY_CC1101_MIN_POWER_LEVEL) *
	            (newPowerPercent / 100.0f));
	CC1101_DEBUG(PSTR("CC1101:SPP:PCT=%u,TX LEVEL=%d\n"), newPowerPercent,
	             newPowerLevel);
	CC1101_txPower(newPowerLevel);
	return true;
}
