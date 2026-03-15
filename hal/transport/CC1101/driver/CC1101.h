
/**
 * @file CC1101.h
 *
 * @defgroup CC1101grp CC1101
 * @ingroup internals
 * @{
 *
 * CC1101 driver-related log messages, format: [!]SYSTEM:[SUB SYSTEM:]MESSAGE
 * - [!] Exclamation mark is prepended in case of error
 *
 * |E| SYS    | SUB  | Message                                | Comment
 * |-|--------|------|----------------------------------------|-----------------------------------------------------------------------------------------
 * | | CC1101 | INIT |                                        | Initialise CC1101 radio
 * | | CC1101 | INIT | GD0PIN=%%u                             | GDO0 pin number
 * |!| CC1101 | INIT | SANCHK FAIL                            | Sanity check failed, check wiring or replace module
 * | | CC1101 | RECV | DATA=%%s,RSSI=%%d (%%d)                | Received data, RSSI and raw RSSI value
 * |!| CC1101 | RECV | OVERFLOW                               | RX FIFO overflowed
 * |!| CC1101 | CSN  | TMOUT=%%d                              | CSN to SO low timeout
 * | | CC1101 | PTC  | LEVEL=%%d                              | Set TX power level
 * | | CC1101 | SAC  | SEND ACK,TO=%%d,SEQ=%%u,RSSI=%%d,LQI=%%u | Send ACK to node (TO), sequence number (SEQ), RSSI of received message (RSSI), LQI of message (LQI)
 * | | CC1101 | ATC  | cR=%%d, tR=%%d, pL=%%d, newPL=%%d       | Adjust TX level, current RSSI (cR), target RSSI (tR), current power level (pL), new power level (newPL)
 * | | CC1101 | SWR  | SEND,TO=%%u,LEN=%%d,SEQ=%%u,RETRY=%%u,NOACK=%%d | Send message to (TO), length (LEN), sequence number (SEQ), retry count (RETRY), noACK flag (NOACK)
 * | | CC1101 | SWR  | ACK FROM=%%u,SEQ=%%u,RSSI=%%d,LQI:%%d  | ACK received from node (FROM), sequence number (SEQ), RSSI, LQI
 * |!| CC1101 | SWR  | NACK                                   | No ACK received
 * |!| CC1101 | CCA  | FAIL                                   | Could not send, channel active
 * |!| CC1101 | TXTMOUT|                                      | Transmit timeout
 * | | CC1101 | SPP  | PCT=%%u,TX LEVEL=%%d                   | Set TX level percent (PCT), TX level (LEVEL)
 * | | CC1101 | FREQ | 0x%%02x%%02x%%02x                      | Frequency set to value

Options for modem config (MY_CC1101_MODEM_CONFIGURATION)

| Config                      | Bandwidth | Baud   | Deviation | Comment                      |
|-----------------------------|-----------|--------|-----------|------------------------------|
| CC1101_MODEM_S_BR4_8_FD20   |  135KHz   |   4800 |  19kHz    | Default, low speed, reliable |
| CC1101_MODEM_M_BR38_4_FD20  |  200KHz   |  38400 |  47kHz    | Higher speed, less reliable  |
| CC1101_MODEM_F_BR100_FD50   |  200KHz   |   100k |  47kHz    | Much faster, less reliable   |
| CC1101_MODEM_XF_BR250_FD125 |  320KHz   |   250k |  125kHz   | Fasest, least reliable       |

 *
 * @brief API declaration for CC1101
 *
 */

#ifndef _CC1101_h
#define _CC1101_h

#include "CC1101Registers.h"
#include <stdint.h>

// CC1101 hardware defaults
#if !defined(CC1101_SPI)
#define CC1101_SPI hwSPI //!< default SPI
#endif

// default PIN assignments, can be overridden
#if defined(ARDUINO_ARCH_AVR)
#define DEFAULT_CC1101_GD0_PIN (2) //!< DEFAULT_CC1101_GD0_PIN
#define DEFAULT_CC1101_GD2_PIN (3) //!< DEFAULT_CC1101_GD2_PIN
#endif

#ifndef DEFAULT_CC1101_CS_PIN
#define DEFAULT_CC1101_CS_PIN (SS) //!< DEFAULT_CC1101_CS_PIN
#endif

#define CC1101_CRYSTAL_HZ 26000000LL

#define CC1101_315MHZ 315000000LL
#define CC1101_433MHZ 433100000LL
#define CC1101_868MHZ 868100000LL
#define CC1101_915MHZ 915100000LL
#define CC1101_FREQ_VAL (((MY_CC1101_FREQUENCY)*65536LL) / CC1101_CRYSTAL_HZ)
#define CC1101_FREQ_BYTE0 ((CC1101_FREQ_VAL >> 16) & 0xFF)
#define CC1101_FREQ_BYTE1 ((CC1101_FREQ_VAL >> 8) & 0xFF)
#define CC1101_FREQ_BYTE2 (CC1101_FREQ_VAL & 0xFF)

#if (MY_CC1101_FREQUENCY == CC1101_315MHZ)
#define CC1101_PA_TABLE PA_TABLE_315
#elif (MY_CC1101_FREQUENCY == CC1101_433MHZ)
#define CC1101_PA_TABLE PA_TABLE_433
#elif (MY_CC1101_FREQUENCY == CC1101_868MHZ)
#define CC1101_PA_TABLE PA_TABLE_868
#elif (MY_CC1101_FREQUENCY == CC1101_915MHZ)
#define CC1101_PA_TABLE PA_TABLE_868
#else
#error Unsupported value for MY_CC1101_FREQUENCY
#endif

// Power levels represent which entry in the PA_TABLE to use.
typedef enum cc1101PowerLevel_e {
	CC1101_POWER_0 = 0x0, //!< -30dBm
	CC1101_POWER_1 = 0x1, //!< -20dBm
	CC1101_POWER_2 = 0x2, //!< -15dBm
	CC1101_POWER_3 = 0x3, //!< -10dBm
	CC1101_POWER_4 = 0x4, //!<  0dBm
	CC1101_POWER_5 = 0x5, //!<  5dBm
	CC1101_POWER_6 = 0x6, //!<  7dBm
	CC1101_POWER_7 = 0x7, //!<  10dBm
	CC1101_POWER_AUTO = 0xff, //!< Enable ATC.
} cc1101_powerLevel_t;

#ifndef MY_CC1101_POWER_LEVEL
#ifdef MY_GATEWAY_FEATURE
#define MY_CC1101_POWER_LEVEL CC1101_POWER_6
#else
#define MY_CC1101_POWER_LEVEL CC1101_POWER_AUTO
#endif
#endif

#ifndef MY_CC1101_MIN_POWER_LEVEL
#define MY_CC1101_MIN_POWER_LEVEL (0)
#endif

#ifndef MY_CC1101_MAX_POWER_LEVEL
#define MY_CC1101_MAX_POWER_LEVEL (7)
#endif

#define CC1101_ATC_TARGET_RANGE_DBM (15) //!< ATC target range +/- dBm
#define CC1101_RSSI_OFFSET (74u)          //!< RSSI offset
#define CC1101_TARGET_RSSI (-50)          //!< RSSI target

#define CC1101_MODEM_S_BR4_8_FD20 0 //!< 135KHz bandwidth, 4800 baud, 19kHz deviation
#define CC1101_MODEM_M_BR38_4_FD20 1 //!< 200KHz bandwidth, 38400 baud, 47kHz deviation
#define CC1101_MODEM_F_BR100_FD50 2 //!< 200KHz bandwidth, 100000 baud, 47kHz deviation
#define CC1101_MODEM_XF_BR250_FD125 3 //!< 320KHz bandwidth, 250000 baud, 125kHz deviation

#ifndef MY_CC1101_MODEM_CONFIGURATION
#define MY_CC1101_MODEM_CONFIGURATION CC1101_MODEM_M_BR38_4_FD20
#endif

#if (MY_CC1101_MODEM_CONFIGURATION) == (CC1101_MODEM_S_BR4_8_FD20)
#define MY_CC1101_REG_MDMCFG4_VALUE 0x67 // 135KHz bw, 4800b
#define MY_CC1101_REG_MDMCFG3_VALUE 0x83 // 4800b
#define MY_CC1101_REG_MDMCFG2_VALUE 0x12
#define MY_CC1101_REG_DEVIATN_VALUE 0x34 // 19khz dev.
#elif (MY_CC1101_MODEM_CONFIGURATION) == (CC1101_MODEM_M_BR38_4_FD20)
#define MY_CC1101_REG_MDMCFG4_VALUE 0x8a // 200KHz bw, 38.4kb
#define MY_CC1101_REG_MDMCFG3_VALUE 0x82 // 38.4kb
#define MY_CC1101_REG_MDMCFG2_VALUE 0x12
#define MY_CC1101_REG_DEVIATN_VALUE 0x47 // 47khz dev.
#elif (MY_CC1101_MODEM_CONFIGURATION) == (CC1101_MODEM_F_BR100_FD50)
#define MY_CC1101_REG_MDMCFG4_VALUE 0x8B // 200KHz bw, 100kb
#define MY_CC1101_REG_MDMCFG3_VALUE 0xF8 // 100kb
#define MY_CC1101_REG_MDMCFG2_VALUE 0x12
#define MY_CC1101_REG_DEVIATN_VALUE 0x47 // 47khz dev.
#elif (MY_CC1101_MODEM_CONFIGURATION) == (CC1101_MODEM_XF_BR250_FD125)
#define MY_CC1101_REG_MDMCFG4_VALUE 0x5D // 320KHz bw, 250kb
#define MY_CC1101_REG_MDMCFG3_VALUE 0x3B // 250kb
#define MY_CC1101_REG_MDMCFG2_VALUE 0x12
#define MY_CC1101_REG_DEVIATN_VALUE 0x62 // 125khz dev.
#else
#error Invalid setting for MY_CC1101_MODEM_CONFIGURATION.
#endif

#define CC1101_MAX_PACKET_LEN (0x40u) //!< Maximum packet size that can be carried by the modem.
#define CC1101_PREAMBLE_LENGTH (8u) //!< Preamble length, default=8
#define CC1101_BROADCAST_ADDRESS 0xFF

#ifndef MY_CC1101_SYNC_WORD
#define MY_CC1101_SYNC_WORD 0xF543 //!< Sync word, can be used to separate networks.
#endif

#define CC1101_TX_RETRIES (5u) //!< Retries in case of fail to transmit (eg CCA)

#if !defined(MY_CC1101_TX_TIMEOUT_MS)
#define MY_CC1101_TX_TIMEOUT_MS                                                \
	(50ul) //!< Timeout before TX is considered failed.
#endif

#if !defined(MY_CC1101_TX_RETRY_DELAY_MS)
#define MY_CC1101_TX_RETRY_DELAY_MS                                            \
	(50l) //!< Delay before reattempting transmit.
#endif

#define CC1101_SEND_RETRIES (4u) //!< Retries in case ACK is not received.

#if !defined(MY_CC1101_SEND_TIMEOUT_MS)
#define MY_CC1101_SEND_TIMEOUT_MS (500ul) //!< How long to wait for an ACK.
#endif

#if !defined(MY_CC1101_ACK_SEND_DELAY_MS)
#define MY_CC1101_ACK_SEND_DELAY_MS                                            \
	5 //!< Delay before sending ACK packet, to give slow nodes a chance to flip to
//!< RX.
#endif

#if !defined(MY_CC1101_CSN_TIMEOUT_MS)
#define MY_CC1101_CSN_TIMEOUT_MS (2ul) //!< CSN to SO low timeout
#endif

#if (MY_CC1101_MAX_POWER_LEVEL) <= (MY_CC1101_MIN_POWER_LEVEL)
#error MY_CC1101_MAX_POWER_LEVEL has to be bigger than MY_CC1101_MIN_POWER_LEVEL
#endif

#if !defined(CC1101_RETRY_TIMEOUT_MS)
// air-time approximation for timeout, 1 hop ~15 bytes payload - adjust if
// needed <fill in>
#define CC1101_RETRY_TIMEOUT_MS                                                \
	(1000ul) //!< Timeout for ACK, adjustments needed if modem configuration
//!< changed (air time different)
#endif

#define CC1101_WRITE_SINGLE 0x00
#define CC1101_WRITE_BURST 0x40
#define CC1101_READ_SINGLE 0x80
#define CC1101_READ_BURST 0xC0

/**
 * @brief Sequence number data type
 */
typedef uint16_t
cc1101_sequenceNumber_t; // will eventually change to uint8_t in 3.0
/**
 * @brief RSSI data type
 */
typedef int8_t cc1101_RSSI_t;
/**
 * @brief LQI data type
 */
typedef uint8_t cc1101_LQI_t;
/**
 * @brief Control flag data type
 */
typedef union {
	struct {
		uint8_t : 5;            //!< reserved
		bool ackRssiReport : 1; //!< ackRssiReport
		bool ackReceived : 1;   //!< ackReceived
		bool ackRequested : 1;  //!< ackRequested
	} fields;                 //!< fields
	uint8_t values;           //!< values
} cc1101_controlFlags_t;

/**
 * @brief Chip status byte
 */
typedef uint8_t cc1101_status_t;

/**
 * @brief cc1101 header
 */
typedef struct {
	// First two bytes must be length/address per the CC1101 packet format.
	uint8_t length;                     //!< Length of data - 1 byte
	uint8_t recipient;                  //!< Payload recipient
	uint8_t version;                    //!< Header version
	uint8_t sender;                     //!< Payload sender
	cc1101_controlFlags_t controlFlags; //!< Control flags, used for ACK
	cc1101_sequenceNumber_t
	sequenceNumber; //!< Packet sequence number, used for ACK
} __attribute__((packed)) cc1101_header_t;

/**
 * @brief cc1101 ACK packet structure
 */
typedef struct {
	cc1101_sequenceNumber_t sequenceNumber; //!< sequence number
	cc1101_RSSI_t RSSI;                     //!< RSSI
	cc1101_LQI_t LQI;                       //!< LQI
} __attribute__((packed)) cc1101_ack_t;

#define CC1101_HEADER_LEN                                                      \
	sizeof(cc1101_header_t) //!< Size header inside payload
#define CC1101_MAX_PAYLOAD_LEN                                                 \
	(CC1101_MAX_PACKET_LEN - CC1101_HEADER_LEN) //!< Max payload length

#define CC1101_PACKET_HEADER_VERSION (1u) //!< CC1101 packet header version
#define CC1101_MIN_PACKET_HEADER_VERSION                                       \
	(1u) //!< Minimal CC1101 packet header version

/**
 * @brief Packet structure
 */
typedef struct {
	union {
		struct {
			cc1101_header_t header; //!< header
			union {
				uint8_t payload[CC1101_MAX_PAYLOAD_LEN]; //!< Payload, i.e. MySensors
				//!< message
				cc1101_ack_t ACK;                        //!< Union: ACK
			};
		};
		uint8_t data[CC1101_MAX_PACKET_LEN]; //!< RAW
	};
	uint8_t payloadLen; //!< Length of payload (excluding header)
	cc1101_RSSI_t RSSI; //!< RSSI of current packet, RSSI = value - 137
	cc1101_LQI_t LQI;   //!< LQI of current packet
} __attribute__((packed)) cc1101_packet_t;

/**
 * @brief CC1101 operation Modes
 */
typedef enum {
	CC1101_MODE_SLEEP = 0x00, //!< CC1101 is in sleep mode
	CC1101_MODE_IDLE,         //!< CC1101 is in idle mode.
	CC1101_MODE_FS,           //!< CC1101 is in frequency synthesis mode
	CC1101_MODE_TX,           //!< CC1101 is in transmit mode
	CC1101_MODE_RX,           //!< CC1101 is in receive mode
	CC1101_MODE_CCA           //!< CC1101 is in clear channel assessment mode
} cc1101_radioModes_t;

/**
 * @brief CC1101 internal variables
 */
typedef struct {
	uint8_t address;                          //!< Node address
	cc1101_packet_t currentPacket;            //!< Buffer for current packet
	cc1101_packet_t lastAck;                  //!< Last ack received.
	cc1101_sequenceNumber_t txSequenceNumber; //!< CC1101_txSequenceNumber
	cc1101_powerLevel_t powerLevel;           //!< TX power level index
	cc1101_radioModes_t radioMode;            //!< Current radio mode
	cc1101_RSSI_t targetRSSI;                 //!< ATC target power level
	cc1101_status_t chipStatus;               //<! CC1101 status byte.
	bool ATCenabled;                          //!< ATC enabled
	volatile bool ackReceived : 1;            //!< ACK received
	volatile bool dataReceived : 1;           //!< Data received
	volatile bool txComplete : 1;             //!< Transmission complete
	volatile bool irqFired : 1;               //!< irq occured
} cc1101_internal_t;

/**
 * @brief Sends a command to the CC1101
 * @param command CC1101 command
 **/
static void CC1101_sendCommand(cc1101_commands_t command);

/**
 * @brief Reads register from the CC1101
 * @param reg register address
 **/
static uint8_t CC1101_readRegister(cc1101_registers_t reg);

/**
 * @brief Reads values from a CC1101 register
 * @param address register address
 * @param buffer buffer to populate
 * @param size size to read
 **/
static void CC1101_readRegisterBurst(cc1101_registers_t reg, uint8_t *buffer,
                                     uint8_t size);

/**
 * @brief Writes values to a CC1101 register
 * @param address register address
 * @param buffer buffer to write
 * @param size size of buffer to write
 **/
static void CC1101_sendRegisterBurst(cc1101_registers_t reg,
                                     const uint8_t *buffer, uint8_t size);

/**
 * @brief Writes a single byte to a CC1101 register
 * @param address register address
 * @param value value to write
 */
static void CC1101_sendRegister(cc1101_registers_t address, uint8_t value);

/**
 * @brief Initialise the driver transport hardware and software
 * @return True if initialisation succeeded
 */
static bool CC1101_initialise();

/**
 * @brief Configure chip registers.
 */
static void CC1101_configure();

/**
 * @brief Set CSN low, wait for ready
 * @return True if SO then went low within MY_CC1101_CSN_TIMEOUT_MS
 **/
static bool CC1101_csnLow();

/**
 * @brief Set CSN high.
 **/
static void CC1101_csnHigh();

/**
 * @brief Wake up the CC1101
 **/
static void CC1101_wakeUp();

/**
 * @brief Send the CC1101 to idle
 */
static void CC1101_idle();

/**
 * @brief Sends the CC1101 to sleep mode
 */
static void CC1101_sleep();

/**
 * @brief Handles CC1101 fired irq
 */
static void CC1101_interruptHandler();

/**
 * @brief Handles CC1101 events
 */
static void CC1101_handle();

/**
 * @brief Sets TX power of the module
 * @param The output power level.
 */
static void CC1101_txPower(cc1101_powerLevel_t power);

/**
 * @brief Sets the operating frequency
 */
static void CC1101_setFrequency();

/**
 * @brief Checks if communication to the CC1101 works
 * @return True if communication is OK
 */
static bool CC1101_sanityCheck();

/**
 * @brief set the node address
 * @param address the device address
 */
static void CC1101_setAddress(uint8_t address);

/**
 * @brief get the node address
 * @return returns the node address
 */
static uint8_t CC1101_getAddress(void);

/**
 * @brief CC1101_sendWithRetry
 * @param recipient recipient of the message
 * @param buffer buffer to send
 * @param bufferSize size of the buffer
 * @param noACK if set to true, no ack will be requested
 * @return True if packet successfully sent
 */
static bool CC1101_sendWithRetry(const uint8_t recipient, const void *buffer,
                                 const uint8_t bufferSize, const bool noACK);

/**
 * @brief Prepare packet to send
 * @param recipient recipient of the packet
 * @param data data of the packet
 * @param len length of the data
 * @param flags flags of the packet
 * @return True if packet sent
 */
static bool CC1101_send(const uint8_t recipient, uint8_t *data,
                        const uint8_t len, const cc1101_controlFlags_t flags);
/**
 * @brief CC1101_sendPacket
 * @param packet the packet to send
 * @return True if frame sent
 */
static bool CC1101_sendPacket(cc1101_packet_t *packet);

/**
 * @brief Sends buffer to the cc1101 tx buffer
 * @param buffer buffer to send
 * @param size size of the buffer
 */
static void CC1101_sendFifo(const uint8_t *buffer, const uint8_t size);

/**
 * @brief Reads into buffer from the cc1101 rx buffer
 * @param buffer buffer to read into
 * @param size size of the buffer
 */
static void CC1101_readFifo(uint8_t *buffer, const uint8_t size);

/**
 * @brief Puts CC1101 into transmitting mode
 */
static void CC1101_tx();

/**
 * @brief Puts CC1101 into receiving mode
 */
static void CC1101_rx();

/**
 * @brief Checks if an unprocessed packet is available
 * @return true if an unprocessed packet is available
 */
static bool CC1101_packetAvailable();

/**
 * @brief Writes available data to the given buffer
 * @param buffer Pointer to the buffer where to write the data
 * @param bufrerSize size of the receiving buffer
 */
static uint8_t CC1101_getData(uint8_t *buffer, const uint8_t bufferSize);

/**
 * @brief sends an acknowledge to the sender
 * @param recipient Recipient of the ack
 * @param sequenceNumber sequence number of the packet we acknowledge
 * @param RSSI RSSI of the package we acknowledge
 * @param LQI LQI of the package we acknowledge
 */
static void CC1101_sendAck(const uint8_t recipient,
                           const cc1101_sequenceNumber_t sequenceNumber,
                           const cc1101_RSSI_t RSSI, const cc1101_LQI_t LQI);

/**
 * @brief sets power level according to target RSSI, based on RSSI reported by
 * ACK
 */
static void CC1101_ATC();

/**
 * @brief sets the ATC mode and target RSSI
 * @param onOff true to switch on ATC
 * @param targetRSSI the desired RSSI
 */
static void CC1101_setATC(bool onOff, int8_t targetRSSI);

/**
 * @brief get our own RSSI as reported by ACK
 * @return own RSSI in dBm
 */
static int16_t CC1101_getSendingRSSI();

/**
 * @brief get remote RSSI
 * @return remote RSSI in dBm
 */
static int16_t CC1101_getReceivingRSSI();

/**
 * @brief get TX power level in dBm
 * @return TX power level in dBm
 **/
static int8_t CC1101_getTxPowerLevel();

/**
 * @brief get TX power level in percent
 * @return TX power level in percent
 */
static uint8_t CC1101_getTxPowerPercent();

/**
 * @brief set TX power level in percent
 * @param newPowerPercent TX power level in percent
 */
static bool CC1101_setTxPowerPercent(const uint8_t newPowerPercent);

/** @}*/
#endif
