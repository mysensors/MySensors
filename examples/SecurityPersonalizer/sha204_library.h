#include "Arduino.h"

#ifndef sha204_library_H
#define sha204_library_H

/* bitbang_config.h */

#define PORT_ACCESS_TIME  		(630)	//! time it takes to toggle the pin at CPU clock of 16 MHz (ns)
#define START_PULSE_WIDTH  		(4340)	//! width of start pulse (ns)
#define BIT_DELAY	      		(4)		//! delay macro for width of one pulse (start pulse or zero pulse, in ns)
#define RX_TX_DELAY        		(15)		//! turn around time when switching from receive to transmit
#define START_PULSE_TIME_OUT	(255)	//! This value is decremented while waiting for the falling edge of a start pulse.
#define ZERO_PULSE_TIME_OUT		(26)	//! This value is decremented while waiting for the falling edge of a zero pulse.

/* swi_phys.h */

#define SWI_FUNCTION_RETCODE_SUCCESS     ((uint8_t) 0x00) //!< Communication with device succeeded.
#define SWI_FUNCTION_RETCODE_TIMEOUT     ((uint8_t) 0xF1) //!< Communication timed out.
#define SWI_FUNCTION_RETCODE_RX_FAIL     ((uint8_t) 0xF9) //!< Communication failed after at least one byte was received.

/* sha204_physical.h */

#define SHA204_RSP_SIZE_MIN          ((uint8_t)  4)  //!< minimum number of bytes in response
#define SHA204_RSP_SIZE_MAX          ((uint8_t) 35)  //!< maximum size of response packet
#define SHA204_BUFFER_POS_COUNT      (0)             //!< buffer index of count byte in command or response
#define SHA204_BUFFER_POS_DATA       (1)             //!< buffer index of data in response
#define SHA204_WAKEUP_PULSE_WIDTH    (uint8_t) (6.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5)	//! width of Wakeup pulse in 10 us units
#define SHA204_WAKEUP_DELAY          (uint8_t) (3.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5)	//! delay between Wakeup pulse and communication in ms

/* sha204_swi.c */
#define SHA204_SWI_FLAG_CMD     ((uint8_t) 0x77) //!< flag preceding a command
#define SHA204_SWI_FLAG_TX      ((uint8_t) 0x88) //!< flag requesting a response
#define SHA204_SWI_FLAG_IDLE    ((uint8_t) 0xBB) //!< flag requesting to go into Idle mode
#define SHA204_SWI_FLAG_SLEEP   ((uint8_t) 0xCC) //!< flag requesting to go into Sleep mode

/* sha204_comm_marshaling.h */
// command op-code definitions
#define SHA204_CHECKMAC                 ((uint8_t) 0x28)       //!< CheckMac command op-code
#define SHA204_DERIVE_KEY               ((uint8_t) 0x1C)       //!< DeriveKey command op-code
#define SHA204_DEVREV                   ((uint8_t) 0x30)       //!< DevRev command op-code
#define SHA204_GENDIG                   ((uint8_t) 0x15)       //!< GenDig command op-code
#define SHA204_HMAC                     ((uint8_t) 0x11)       //!< HMAC command op-code
#define SHA204_LOCK                     ((uint8_t) 0x17)       //!< Lock command op-code
#define SHA204_MAC                      ((uint8_t) 0x08)       //!< MAC command op-code
#define SHA204_NONCE                    ((uint8_t) 0x16)       //!< Nonce command op-code
#define SHA204_PAUSE                    ((uint8_t) 0x01)       //!< Pause command op-code
#define SHA204_RANDOM                   ((uint8_t) 0x1B)       //!< Random command op-code
#define SHA204_READ                     ((uint8_t) 0x02)       //!< Read command op-code
#define SHA204_UPDATE_EXTRA             ((uint8_t) 0x20)       //!< UpdateExtra command op-code
#define SHA204_WRITE                    ((uint8_t) 0x12)       //!< Write command op-code

// packet size definitions
#define SHA204_RSP_SIZE_VAL             ((uint8_t)  7)         //!< size of response packet containing four bytes of data

// parameter range definitions
#define SHA204_KEY_ID_MAX               ((uint8_t) 15)         //!< maximum value for key id
#define SHA204_OTP_BLOCK_MAX            ((uint8_t)  1)         //!< maximum value for OTP block

// definitions for command packet indexes common to all commands
#define SHA204_COUNT_IDX                ( 0)                   //!< command packet index for count
#define SHA204_OPCODE_IDX               ( 1)                   //!< command packet index for op-code
#define SHA204_PARAM1_IDX               ( 2)                   //!< command packet index for first parameter
#define SHA204_PARAM2_IDX               ( 3)                   //!< command packet index for second parameter
#define SHA204_DATA_IDX                 ( 5)                   //!< command packet index for second parameter

// zone definitions
#define SHA204_ZONE_CONFIG              ((uint8_t)  0x00)      //!< Configuration zone
#define SHA204_ZONE_OTP                 ((uint8_t)  0x01)      //!< OTP (One Time Programming) zone
#define SHA204_ZONE_DATA                ((uint8_t)  0x02)      //!< Data zone
#define SHA204_ZONE_MASK                ((uint8_t)  0x03)      //!< Zone mask
#define SHA204_ZONE_COUNT_FLAG          ((uint8_t)  0x80)      //!< Zone bit 7 set: Access 32 bytes, otherwise 4 bytes.
#define SHA204_ZONE_ACCESS_4            ((uint8_t)     4)      //!< Read or write 4 bytes.
#define SHA204_ZONE_ACCESS_32           ((uint8_t)    32)      //!< Read or write 32 bytes.
#define SHA204_ADDRESS_MASK_CONFIG      (         0x001F)      //!< Address bits 5 to 7 are 0 for Configuration zone.
#define SHA204_ADDRESS_MASK_OTP         (         0x000F)      //!< Address bits 4 to 7 are 0 for OTP zone.
#define SHA204_ADDRESS_MASK             (         0x007F)      //!< Address bit 7 to 15 are always 0.

// CheckMAC command definitions
#define CHECKMAC_MODE_IDX               SHA204_PARAM1_IDX      //!< CheckMAC command index for mode
#define CHECKMAC_KEYID_IDX              SHA204_PARAM2_IDX      //!< CheckMAC command index for key identifier
#define CHECKMAC_CLIENT_CHALLENGE_IDX   SHA204_DATA_IDX        //!< CheckMAC command index for client challenge
#define CHECKMAC_CLIENT_RESPONSE_IDX    (37)                   //!< CheckMAC command index for client response
#define CHECKMAC_DATA_IDX               (69)                   //!< CheckMAC command index for other data
#define CHECKMAC_COUNT                  (84)                   //!< CheckMAC command packet size
#define CHECKMAC_MODE_MASK              ((uint8_t) 0x27)       //!< CheckMAC mode bits 3, 4, 6, and 7 are 0.
#define CHECKMAC_CLIENT_CHALLENGE_SIZE  (32)                   //!< CheckMAC size of client challenge
#define CHECKMAC_CLIENT_RESPONSE_SIZE   (32)                   //!< CheckMAC size of client response
#define CHECKMAC_OTHER_DATA_SIZE        (13)                   //!< CheckMAC size of "other data"

// DeriveKey command definitions
#define DERIVE_KEY_RANDOM_IDX           SHA204_PARAM1_IDX      //!< DeriveKey command index for random bit
#define DERIVE_KEY_TARGETKEY_IDX        SHA204_PARAM2_IDX      //!< DeriveKey command index for target slot
#define DERIVE_KEY_MAC_IDX              SHA204_DATA_IDX        //!< DeriveKey command index for optional MAC
#define DERIVE_KEY_COUNT_SMALL          SHA204_CMD_SIZE_MIN    //!< DeriveKey command packet size without MAC
#define DERIVE_KEY_COUNT_LARGE          (39)                   //!< DeriveKey command packet size with MAC
#define DERIVE_KEY_RANDOM_FLAG          ((uint8_t) 4)          //!< DeriveKey 1. parameter
#define DERIVE_KEY_MAC_SIZE             (32)                   //!< DeriveKey MAC size

// DevRev command definitions
#define DEVREV_PARAM1_IDX               SHA204_PARAM1_IDX      //!< DevRev command index for 1. parameter (ignored)
#define DEVREV_PARAM2_IDX               SHA204_PARAM2_IDX      //!< DevRev command index for 2. parameter (ignored)
#define DEVREV_COUNT                    SHA204_CMD_SIZE_MIN    //!< DevRev command packet size

// GenDig command definitions
#define GENDIG_ZONE_IDX                 SHA204_PARAM1_IDX      //!< GenDig command index for zone
#define GENDIG_KEYID_IDX                SHA204_PARAM2_IDX      //!< GenDig command index for key id
#define GENDIG_DATA_IDX                 SHA204_DATA_IDX        //!< GenDig command index for optional data
#define GENDIG_COUNT                    SHA204_CMD_SIZE_MIN    //!< GenDig command packet size without "other data"
#define GENDIG_COUNT_DATA               (11)                   //!< GenDig command packet size with "other data"
#define GENDIG_OTHER_DATA_SIZE          (4)                    //!< GenDig size of "other data"
#define GENDIG_ZONE_CONFIG              ((uint8_t) 0)          //!< GenDig zone id config
#define GENDIG_ZONE_OTP                 ((uint8_t) 1)          //!< GenDig zone id OTP
#define GENDIG_ZONE_DATA                ((uint8_t) 2)          //!< GenDig zone id data

// HMAC command definitions
#define HMAC_MODE_IDX                   SHA204_PARAM1_IDX      //!< HMAC command index for mode
#define HMAC_KEYID_IDX                  SHA204_PARAM2_IDX      //!< HMAC command index for key id
#define HMAC_COUNT                      SHA204_CMD_SIZE_MIN    //!< HMAC command packet size
#define HMAC_MODE_MASK                  ((uint8_t) 0x74)       //!< HMAC mode bits 0, 1, 3, and 7 are 0.

// Lock command definitions
#define LOCK_ZONE_IDX                   SHA204_PARAM1_IDX      //!< Lock command index for zone
#define LOCK_SUMMARY_IDX                SHA204_PARAM2_IDX      //!< Lock command index for summary
#define LOCK_COUNT                      SHA204_CMD_SIZE_MIN    //!< Lock command packet size
#define LOCK_ZONE_NO_CONFIG             ((uint8_t) 0x01)       //!< Lock zone is OTP or Data
#define LOCK_ZONE_NO_CRC                ((uint8_t) 0x80)       //!< Lock command: Ignore summary.
#define LOCK_ZONE_MASK                  (0x81)                 //!< Lock parameter 1 bits 2 to 6 are 0.

// Mac command definitions
#define MAC_MODE_IDX                    SHA204_PARAM1_IDX      //!< MAC command index for mode
#define MAC_KEYID_IDX                   SHA204_PARAM2_IDX      //!< MAC command index for key id
#define MAC_CHALLENGE_IDX               SHA204_DATA_IDX        //!< MAC command index for optional challenge
#define MAC_COUNT_SHORT                 SHA204_CMD_SIZE_MIN    //!< MAC command packet size without challenge
#define MAC_COUNT_LONG                  (39)                   //!< MAC command packet size with challenge
#define MAC_MODE_BLOCK2_TEMPKEY         ((uint8_t) 0x01)       //!< MAC mode bit   0: second SHA block from TempKey
#define MAC_MODE_BLOCK1_TEMPKEY         ((uint8_t) 0x02)       //!< MAC mode bit   1: first SHA block from TempKey
#define MAC_MODE_SOURCE_FLAG_MATCH      ((uint8_t) 0x04)       //!< MAC mode bit   2: match TempKey.SourceFlag
#define MAC_MODE_PASSTHROUGH            ((uint8_t) 0x07)       //!< MAC mode bit 0-2: pass-through mode
#define MAC_MODE_INCLUDE_OTP_88         ((uint8_t) 0x10)       //!< MAC mode bit   4: include first 88 OTP bits
#define MAC_MODE_INCLUDE_OTP_64         ((uint8_t) 0x20)       //!< MAC mode bit   5: include first 64 OTP bits
#define MAC_MODE_INCLUDE_SN             ((uint8_t) 0x40)       //!< MAC mode bit   6: include serial number
#define MAC_CHALLENGE_SIZE              (32)                   //!< MAC size of challenge
#define MAC_MODE_MASK                   ((uint8_t) 0x77)       //!< MAC mode bits 3 and 7 are 0.

// Nonce command definitions
#define NONCE_MODE_IDX                  SHA204_PARAM1_IDX      //!< Nonce command index for mode
#define NONCE_PARAM2_IDX                SHA204_PARAM2_IDX      //!< Nonce command index for 2. parameter
#define NONCE_INPUT_IDX                 SHA204_DATA_IDX        //!< Nonce command index for input data
#define NONCE_COUNT_SHORT               (27)                   //!< Nonce command packet size for 20 bytes of data
#define NONCE_COUNT_LONG                (39)                   //!< Nonce command packet size for 32 bytes of data
#define NONCE_MODE_MASK                 ((uint8_t) 3)          //!< Nonce mode bits 2 to 7 are 0.
#define NONCE_MODE_SEED_UPDATE          ((uint8_t) 0x00)       //!< Nonce mode: update seed
#define NONCE_MODE_NO_SEED_UPDATE       ((uint8_t) 0x01)       //!< Nonce mode: do not update seed
#define NONCE_MODE_INVALID              ((uint8_t) 0x02)       //!< Nonce mode 2 is invalid.
#define NONCE_MODE_PASSTHROUGH          ((uint8_t) 0x03)       //!< Nonce mode: pass-through
#define NONCE_NUMIN_SIZE                (20)                   //!< Nonce data length
#define NONCE_NUMIN_SIZE_PASSTHROUGH    (32)                   //!< Nonce data length in pass-through mode (mode = 3)

// Pause command definitions
#define PAUSE_SELECT_IDX                SHA204_PARAM1_IDX      //!< Pause command index for Selector
#define PAUSE_PARAM2_IDX                SHA204_PARAM2_IDX      //!< Pause command index for 2. parameter
#define PAUSE_COUNT                     SHA204_CMD_SIZE_MIN    //!< Pause command packet size

// Random command definitions
#define RANDOM_MODE_IDX                 SHA204_PARAM1_IDX      //!< Random command index for mode
#define RANDOM_PARAM2_IDX               SHA204_PARAM2_IDX      //!< Random command index for 2. parameter
#define RANDOM_COUNT                    SHA204_CMD_SIZE_MIN    //!< Random command packet size
#define RANDOM_SEED_UPDATE              ((uint8_t) 0x00)       //!< Random mode for automatic seed update
#define RANDOM_NO_SEED_UPDATE           ((uint8_t) 0x01)       //!< Random mode for no seed update

// Read command definitions
#define READ_ZONE_IDX                   SHA204_PARAM1_IDX      //!< Read command index for zone
#define READ_ADDR_IDX                   SHA204_PARAM2_IDX      //!< Read command index for address
#define READ_COUNT                      SHA204_CMD_SIZE_MIN    //!< Read command packet size
#define READ_ZONE_MASK                  ((uint8_t) 0x83)       //!< Read zone bits 2 to 6 are 0.
#define READ_ZONE_MODE_32_BYTES         ((uint8_t) 0x80)       //!< Read mode: 32 bytes

// UpdateExtra command definitions
#define UPDATE_MODE_IDX                  SHA204_PARAM1_IDX     //!< UpdateExtra command index for mode
#define UPDATE_VALUE_IDX                 SHA204_PARAM2_IDX     //!< UpdateExtra command index for new value
#define UPDATE_COUNT                     SHA204_CMD_SIZE_MIN   //!< UpdateExtra command packet size
#define UPDATE_CONFIG_BYTE_86            ((uint8_t) 0x01)      //!< UpdateExtra mode: update Config byte 86

// Write command definitions
#define WRITE_ZONE_IDX                  SHA204_PARAM1_IDX      //!< Write command index for zone
#define WRITE_ADDR_IDX                  SHA204_PARAM2_IDX      //!< Write command index for address
#define WRITE_VALUE_IDX                 SHA204_DATA_IDX        //!< Write command index for data
#define WRITE_MAC_VS_IDX                ( 9)                   //!< Write command index for MAC following short data
#define WRITE_MAC_VL_IDX                (37)                   //!< Write command index for MAC following long data
#define WRITE_COUNT_SHORT               (11)                   //!< Write command packet size with short data and no MAC
#define WRITE_COUNT_LONG                (39)                   //!< Write command packet size with long data and no MAC
#define WRITE_COUNT_SHORT_MAC           (43)                   //!< Write command packet size with short data and MAC
#define WRITE_COUNT_LONG_MAC            (71)                   //!< Write command packet size with long data and MAC
#define WRITE_MAC_SIZE                  (32)                   //!< Write MAC size
#define WRITE_ZONE_MASK                 ((uint8_t) 0xC3)       //!< Write zone bits 2 to 5 are 0.
#define WRITE_ZONE_WITH_MAC             ((uint8_t) 0x40)       //!< Write zone bit 6: write encrypted with MAC

// Response size definitions
#define CHECKMAC_RSP_SIZE               SHA204_RSP_SIZE_MIN    //!< response size of DeriveKey command
#define DERIVE_KEY_RSP_SIZE             SHA204_RSP_SIZE_MIN    //!< response size of DeriveKey command
#define DEVREV_RSP_SIZE                 SHA204_RSP_SIZE_VAL    //!< response size of DevRev command returns 4 bytes
#define GENDIG_RSP_SIZE                 SHA204_RSP_SIZE_MIN    //!< response size of GenDig command
#define HMAC_RSP_SIZE                   SHA204_RSP_SIZE_MAX    //!< response size of HMAC command
#define LOCK_RSP_SIZE                   SHA204_RSP_SIZE_MIN    //!< response size of Lock command
#define MAC_RSP_SIZE                    SHA204_RSP_SIZE_MAX    //!< response size of MAC command
#define NONCE_RSP_SIZE_SHORT            SHA204_RSP_SIZE_MIN    //!< response size of Nonce command with mode[0:1] = 3
#define NONCE_RSP_SIZE_LONG             SHA204_RSP_SIZE_MAX    //!< response size of Nonce command
#define PAUSE_RSP_SIZE                  SHA204_RSP_SIZE_MIN    //!< response size of Pause command
#define RANDOM_RSP_SIZE                 SHA204_RSP_SIZE_MAX    //!< response size of Random command
#define READ_4_RSP_SIZE                 SHA204_RSP_SIZE_VAL    //!< response size of Read command when reading 4 bytes
#define READ_32_RSP_SIZE                SHA204_RSP_SIZE_MAX    //!< response size of Read command when reading 32 bytes
#define TEMP_SENSE_RSP_SIZE             SHA204_RSP_SIZE_VAL    //!< response size of TempSense command returns 4 bytes
#define UPDATE_RSP_SIZE                 SHA204_RSP_SIZE_MIN    //!< response size of UpdateExtra command
#define WRITE_RSP_SIZE                  SHA204_RSP_SIZE_MIN    //!< response size of Write command

// command timing definitions for minimum execution times (ms)
#define CHECKMAC_DELAY                  ((uint8_t) (12.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define DERIVE_KEY_DELAY                ((uint8_t) (14.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define DEVREV_DELAY                    ((uint8_t) ( 0.4 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define GENDIG_DELAY                    ((uint8_t) (11.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define HMAC_DELAY                      ((uint8_t) (27.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define LOCK_DELAY                      ((uint8_t) ( 5.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define MAC_DELAY                       ((uint8_t) (12.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define NONCE_DELAY                     ((uint8_t) (22.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define PAUSE_DELAY                     ((uint8_t) ( 0.4 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define RANDOM_DELAY                    ((uint8_t) (11.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define READ_DELAY                      ((uint8_t) ( 0.4 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define TEMP_SENSE_DELAY                ((uint8_t) ( 4.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define UPDATE_DELAY                    ((uint8_t) ( 4.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define WRITE_DELAY                     ((uint8_t) ( 4.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))

// command timing definitions for maximum execution times (ms)
#define CHECKMAC_EXEC_MAX                ((uint8_t) (38.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define DERIVE_KEY_EXEC_MAX              ((uint8_t) (62.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define DEVREV_EXEC_MAX                  ((uint8_t) ( 2.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define GENDIG_EXEC_MAX                  ((uint8_t) (43.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define HMAC_EXEC_MAX                    ((uint8_t) (69.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define LOCK_EXEC_MAX                    ((uint8_t) (24.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define MAC_EXEC_MAX                     ((uint8_t) (35.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define NONCE_EXEC_MAX                   ((uint8_t) (60.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define PAUSE_EXEC_MAX                   ((uint8_t) ( 2.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define RANDOM_EXEC_MAX                  ((uint8_t) (50.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define READ_EXEC_MAX                    ((uint8_t) ( 4.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define TEMP_SENSE_EXEC_MAX              ((uint8_t) (11.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define UPDATE_EXEC_MAX                  ((uint8_t) ( 6.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define WRITE_EXEC_MAX                   ((uint8_t) (42.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))

/* from sha204_config.h */

#define CPU_CLOCK_DEVIATION_POSITIVE   (1.01)
#define CPU_CLOCK_DEVIATION_NEGATIVE   (0.99)
#define SHA204_RETRY_COUNT           (1)
#define SWI_RECEIVE_TIME_OUT      ((uint16_t) 163)  //! #START_PULSE_TIME_OUT in us instead of loop counts
#define SWI_US_PER_BYTE           ((uint16_t) 313)  //! It takes 312.5 us to send a byte (9 single-wire bits / 230400 Baud * 8 flag bits).
#define SHA204_SYNC_TIMEOUT       ((uint8_t) 85)//! delay before sending a transmit flag in the synchronization routine
#define SHA204_RESPONSE_TIMEOUT   ((uint16_t) SWI_RECEIVE_TIME_OUT + SWI_US_PER_BYTE)  //! SWI response timeout is the sum of receive timeout and the time it takes to send the TX flag.

/* from sha204_comm.h */

#define SHA204_COMMAND_EXEC_MAX      ((uint8_t) (69.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))  //! maximum command delay
#define SHA204_CMD_SIZE_MIN          ((uint8_t)  7)  //! minimum number of bytes in command (from count byte to second CRC byte)
#ifndef SHA204_CMD_SIZE_MAX
#define SHA204_CMD_SIZE_MAX          ((uint8_t) 84)  //! maximum size of command packet (CheckMac)
#endif
#define SHA204_CRC_SIZE              ((uint8_t)  2)  //! number of CRC bytes
#define SHA204_BUFFER_POS_STATUS     (1)  //! buffer index of status byte in status response
#define SHA204_BUFFER_POS_DATA       (1)  //! buffer index of first data byte in data response
#define SHA204_STATUS_BYTE_WAKEUP    ((uint8_t) 0x11)  //! command parse error
#define SHA204_STATUS_BYTE_PARSE     ((uint8_t) 0x03)  //! command parse error
#define SHA204_STATUS_BYTE_EXEC      ((uint8_t) 0x0F)  //! command execution error
#define SHA204_STATUS_BYTE_COMM      ((uint8_t) 0xFF)  //! communication error

/* EEPROM Addresses */
/* Configuration Zone */
#define ADDRESS_SN03		0	// SN[0:3] are bytes 0->3 of configuration zone
#define ADDRESS_RevNum		4	// bytes 4->7 of config zone are RevNum
#define ADDRESS_SN47		8	// SN[4:7] are bytes 8->11 of config zone
#define ADDRESS_SN8			12	// SN[8] is byte 12 of config zone, should be 0xEE
#define ADDRESS_I2CEN		14	// I2C Enable, bit 0 represents I2C enable status
#define ADDRESS_I2CADD		16	// Defines I2C address of SHA204
#define	ADDRESS_OTPMODE		18	// Sets the One-time-programmable mode
#define	ADDRESS_SELECTOR	19	// Controls writability of Selector

/* Low level HW access macros */
/* function calls is not working, as it will have too much overhead */
#if !defined(ARDUINO_ARCH_AVR) // For everything else than AVR use pinMode / digitalWrite
#define SHA204_SET_OUTPUT() pinMode(device_pin, OUTPUT)
#define SHA204_SET_INPUT() pinMode(device_pin, INPUT)
#define SHA204_POUT_HIGH() digitalWrite(device_pin, HIGH)
#define SHA204_POUT_LOW() digitalWrite(device_pin, LOW)
#define SHA204_PIN_READ() digitalRead(device_pin)
#else
#define SHA204_SET_INPUT()  *device_port_DDR &= ~device_pin
#define SHA204_SET_OUTPUT() *device_port_DDR |= device_pin
#define SHA204_POUT_HIGH() *device_port_OUT |= device_pin
#define SHA204_POUT_LOW() *device_port_OUT &= ~device_pin
#define SHA204_PIN_READ() (*device_port_IN & device_pin)
#endif

/**
 * atsha204Class class
 */
class atsha204Class
{
private:
	uint8_t device_pin;
#ifdef ARDUINO_ARCH_AVR
	volatile uint8_t *device_port_DDR, *device_port_OUT, *device_port_IN;
#endif
	void sha204c_calculate_crc(uint8_t length, uint8_t *data, uint8_t *crc);
	uint8_t sha204c_check_crc(uint8_t *response);
	void swi_set_signal_pin(uint8_t is_high);
	uint8_t swi_receive_bytes(uint8_t count, uint8_t *buffer);
	uint8_t swi_send_bytes(uint8_t count, uint8_t *buffer);
	uint8_t swi_send_byte(uint8_t value);
	uint8_t sha204p_receive_response(uint8_t size, uint8_t *response);
	uint8_t sha204p_wakeup();
	uint8_t sha204p_send_command(uint8_t count, uint8_t * command);
	uint8_t sha204p_sleep();
	uint8_t sha204p_resync(uint8_t size, uint8_t *response);


public:
	/**
	 * @brief      Constructor
	 *
	 * @param[in]  pin   The pin to use for communication
	 */
	explicit atsha204Class(uint8_t pin);

	/**
	 * @brief      Wake up device
	 *
	 * @param      response  The response from the device
	 *
	 * @return     Error code (SHA204_SUCCESS if OK)
	 */
	uint8_t sha204c_wakeup(uint8_t *response);

	/**
	 * @brief      Send and receive data
	 *
	 * @param      tx_buffer          The transmit buffer
	 * @param[in]  rx_size            The receive size
	 * @param      rx_buffer          The receive buffer
	 * @param[in]  execution_delay    The execution delay
	 * @param[in]  execution_timeout  The execution timeout
	 *
	 * @return     Error code (SHA204_SUCCESS if OK)
	 */
	uint8_t sha204c_send_and_receive(uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer,
	                                 uint8_t execution_delay, uint8_t execution_timeout);

	/**
	 * @brief      Resyncronize the device
	 *
	 * @param[in]  size      The size of the response buffer
	 * @param      response  The response
	 *
	 * @return     Error code (SHA204_SUCCESS if OK)
	 */
	uint8_t sha204c_resync(uint8_t size, uint8_t *response);

	/**
	 * @brief      Generate random data
	 *
	 * @param      tx_buffer  The transmit buffer
	 * @param      rx_buffer  The receive buffer
	 * @param[in]  mode       The mode
	 *
	 * @return     Error code (SHA204_SUCCESS if OK)
	 */
	uint8_t sha204m_random(uint8_t * tx_buffer, uint8_t * rx_buffer, uint8_t mode);

	/**
	 * @brief      Read device revision
	 *
	 * @param      tx_buffer  The transmit buffer
	 * @param      rx_buffer  The receive buffer
	 *
	 * @return     Error code (SHA204_SUCCESS if OK)
	 */
	uint8_t sha204m_dev_rev(uint8_t *tx_buffer, uint8_t *rx_buffer);

	/**
	 * @brief      Read from device
	 *
	 * @param      tx_buffer  The transmit buffer
	 * @param      rx_buffer  The receive buffer
	 * @param[in]  zone       The zone
	 * @param[in]  address    The address
	 *
	 * @return     Error code (SHA204_SUCCESS if OK)
	 */
	uint8_t sha204m_read(uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t zone, uint16_t address);

	/**
	 * @brief      Execute command
	 *
	 * @param[in]  op_code    The operation code
	 * @param[in]  param1     The parameter 1
	 * @param[in]  param2     The parameter 2
	 * @param[in]  datalen1   The datalen 1
	 * @param      data1      The data 1
	 * @param[in]  datalen2   The datalen 2
	 * @param      data2      The data 2
	 * @param[in]  datalen3   The datalen 3
	 * @param      data3      The data 3
	 * @param[in]  tx_size    The transmit size
	 * @param      tx_buffer  The transmit buffer
	 * @param[in]  rx_size    The receive size
	 * @param      rx_buffer  The receive buffer
	 *
	 * @return     Error code (SHA204_SUCCESS if OK)
	 */
	uint8_t sha204m_execute(uint8_t op_code, uint8_t param1, uint16_t param2,
	                        uint8_t datalen1, uint8_t *data1, uint8_t datalen2, uint8_t *data2, uint8_t datalen3,
	                        uint8_t *data3,
	                        uint8_t tx_size, uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer);

	/**
	 * @brief      Validate parameters
	 *
	 * @param[in]  op_code    The operation code
	 * @param[in]  param1     The parameter 1
	 * @param[in]  param2     The parameter 2
	 * @param[in]  datalen1   The datalen 1
	 * @param      data1      The data 1
	 * @param[in]  datalen2   The datalen 2
	 * @param      data2      The data 2
	 * @param[in]  datalen3   The datalen 3
	 * @param      data3      The data 3
	 * @param[in]  tx_size    The transmit size
	 * @param      tx_buffer  The transmit buffer
	 * @param[in]  rx_size    The receive size
	 * @param      rx_buffer  The receive buffer
	 *
	 * @return     Error code (SHA204_SUCCESS if OK)
	 */
	uint8_t sha204m_check_parameters(uint8_t op_code, uint8_t param1, uint16_t param2,
	                                 uint8_t datalen1, uint8_t *data1, uint8_t datalen2, uint8_t *data2, uint8_t datalen3,
	                                 uint8_t *data3,
	                                 uint8_t tx_size, uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer);

	/**
	 * @brief      Gets the serial number.
	 *
	 * @param      response  The response
	 *
	 * @return     The serial number
	 */
	uint8_t getSerialNumber(uint8_t *response);

	/**
	 * @brief      Calculates and update crc.
	 *
	 * @param[in]  length       The length
	 * @param      data         The data
	 * @param[in]  current_crc  The current crc
	 *
	 * @return     The updated crc
	 */
	uint16_t calculateAndUpdateCrc(uint8_t length, uint8_t *data, uint16_t current_crc);

};

#endif
