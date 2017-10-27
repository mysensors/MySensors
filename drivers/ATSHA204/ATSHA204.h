#ifndef ATSHA204_H
#define ATSHA204_H
#if !DOXYGEN
#include <Arduino.h>

/* This is a scaled down variant of the ATSHA204 library, tweaked to meet the specific needs of the MySensors library. */

/* Library return codes */
#define SHA204_SUCCESS              ((uint8_t)  0x00) //!< Function succeeded.
#define SHA204_PARSE_ERROR          ((uint8_t)  0xD2) //!< response status byte indicates parsing error
#define SHA204_CMD_FAIL             ((uint8_t)  0xD3) //!< response status byte indicates command execution error
#define SHA204_STATUS_CRC           ((uint8_t)  0xD4) //!< response status byte indicates CRC error
#define SHA204_STATUS_UNKNOWN       ((uint8_t)  0xD5) //!< response status byte is unknown
#define SHA204_FUNC_FAIL            ((uint8_t)  0xE0) //!< Function could not execute due to incorrect condition / state.
#define SHA204_GEN_FAIL             ((uint8_t)  0xE1) //!< unspecified error
#define SHA204_BAD_PARAM            ((uint8_t)  0xE2) //!< bad argument (out of range, null pointer, etc.)
#define SHA204_INVALID_ID           ((uint8_t)  0xE3) //!< invalid device id, id not set
#define SHA204_INVALID_SIZE         ((uint8_t)  0xE4) //!< Count value is out of range or greater than buffer size.
#define SHA204_BAD_CRC              ((uint8_t)  0xE5) //!< incorrect CRC received
#define SHA204_RX_FAIL              ((uint8_t)  0xE6) //!< Timed out while waiting for response. Number of bytes received is > 0.
#define SHA204_RX_NO_RESPONSE       ((uint8_t)  0xE7) //!< Not an error while the Command layer is polling for a command response.
#define SHA204_RESYNC_WITH_WAKEUP   ((uint8_t)  0xE8) //!< re-synchronization succeeded, but only after generating a Wake-up

#define SHA204_COMM_FAIL            ((uint8_t)  0xF0) //!< Communication with device failed. Same as in hardware dependent modules.
#define SHA204_TIMEOUT              ((uint8_t)  0xF1) //!< Timed out while waiting for response. Number of bytes received is 0.

/* bitbang_config.h */

#define PORT_ACCESS_TIME  		(630)	//! time it takes to toggle the pin at CPU clock of 16 MHz (ns)
#define START_PULSE_WIDTH  		(4340)	//! width of start pulse (ns)
#define BIT_DELAY	      		(4)		//! delay macro for width of one pulse (start pulse or zero pulse, in ns)
#define RX_TX_DELAY        		(15)	//! turn around time when switching from receive to transmit
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
#define SHA204_GENDIG                   ((uint8_t) 0x15)       //!< GenDig command op-code
#define SHA204_HMAC                     ((uint8_t) 0x11)       //!< HMAC command op-code
#define SHA204_NONCE                    ((uint8_t) 0x16)       //!< Nonce command op-code
#define SHA204_RANDOM                   ((uint8_t) 0x1B)       //!< Random command op-code
#define SHA204_READ                     ((uint8_t) 0x02)       //!< Read command op-code
#define SHA204_SHA                      ((uint8_t) 0x47)       //!< SHA command op-code
#define SHA204_WRITE                    ((uint8_t) 0x12)       //!< Write command op-code

// packet size definitions
#define SHA204_RSP_SIZE_VAL             ((uint8_t)  7)         //!< size of response packet containing four bytes of data

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
#define HMAC_MODE_SOURCE_FLAG_MATCH     ((uint8_t) 0x04)       //!< HMAC mode bit  2: match TempKey.SourceFlag

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

// SHA command definitions
#define SHA_MODE_IDX                    SHA204_PARAM1_IDX      //!< SHA command index for mode
#define SHA_PARAM2_IDX                  SHA204_PARAM2_IDX      //!< SHA command index for 2. parameter
#define SHA_COUNT_SHORT                 SHA204_CMD_SIZE_MIN    //!< SHA command packet size for init
#define SHA_COUNT_LONG                  (71)                   //!< SHA command packet size for calculation
#define SHA_MSG_SIZE                    (64)                   //!< SHA message data size
#define SHA_INIT                        ((uint8_t) 0x00)       //!< SHA mode for init
#define SHA_CALC                        ((uint8_t) 0x01)       //!< SHA mode for calculation

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
#define GENDIG_RSP_SIZE                 SHA204_RSP_SIZE_MIN    //!< response size of GenDig command
#define HMAC_RSP_SIZE                   SHA204_RSP_SIZE_MAX    //!< response size of HMAC command
#define NONCE_RSP_SIZE_SHORT            SHA204_RSP_SIZE_MIN    //!< response size of Nonce command with mode[0:1] = 3
#define NONCE_RSP_SIZE_LONG             SHA204_RSP_SIZE_MAX    //!< response size of Nonce command
#define RANDOM_RSP_SIZE                 SHA204_RSP_SIZE_MAX    //!< response size of Random command
#define READ_4_RSP_SIZE                 SHA204_RSP_SIZE_VAL    //!< response size of Read command when reading 4 bytes
#define READ_32_RSP_SIZE                SHA204_RSP_SIZE_MAX    //!< response size of Read command when reading 32 bytes
#define SHA_RSP_SIZE_SHORT              SHA204_RSP_SIZE_MIN    //!< response size of SHA command with mode[0:1] = 0
#define SHA_RSP_SIZE_LONG               SHA204_RSP_SIZE_MAX    //!< response size of SHA command
#define WRITE_RSP_SIZE                  SHA204_RSP_SIZE_MIN    //!< response size of Write command

// command timing definitions for minimum execution times (ms)
#define GENDIG_DELAY                    ((uint8_t) (11.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define HMAC_DELAY                      ((uint8_t) (27.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define NONCE_DELAY                     ((uint8_t) (22.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define RANDOM_DELAY                    ((uint8_t) (11.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define READ_DELAY                      ((uint8_t) ( 0.4 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define SHA_DELAY                       ((uint8_t) (11.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))
#define WRITE_DELAY                     ((uint8_t) ( 4.0 * CPU_CLOCK_DEVIATION_NEGATIVE - 0.5))

// command timing definitions for maximum execution times (ms)
#define GENDIG_EXEC_MAX                  ((uint8_t) (43.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define HMAC_EXEC_MAX                    ((uint8_t) (69.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define NONCE_EXEC_MAX                   ((uint8_t) (60.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define RANDOM_EXEC_MAX                  ((uint8_t) (50.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define READ_EXEC_MAX                    ((uint8_t) ( 4.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
#define SHA_EXEC_MAX                     ((uint8_t) (22.0 * CPU_CLOCK_DEVIATION_POSITIVE + 0.5))
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
#define SHA204_CMD_SIZE_MAX          ((uint8_t) SHA_COUNT_LONG)  //! maximum size of command packet (SHA)
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
#define ADDRESS_SN03        0   // SN[0:3] are bytes 0->3 of configuration zone
#define ADDRESS_RevNum      4   // bytes 4->7 of config zone are RevNum
#define ADDRESS_SN47        8   // SN[4:7] are bytes 8->11 of config zone
#define ADDRESS_SN8         12  // SN[8] is byte 12 of config zone, should be 0xEE
#define ADDRESS_I2CEN       14  // I2C Enable, bit 0 represents I2C enable status
#define ADDRESS_I2CADD      16  // Defines I2C address of SHA204
#define ADDRESS_OTPMODE     18  // Sets the One-time-programmable mode
#define ADDRESS_SELECTOR    19  // Controls writability of Selector

#define SHA204_SERIAL_SZ    9   // The number of bytes the serial number consists of

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

void atsha204_init(uint8_t pin);
void atsha204_idle(void);
void atsha204_sleep(void);
uint8_t atsha204_wakeup(uint8_t *response);
uint8_t atsha204_execute(uint8_t op_code, uint8_t param1, uint16_t param2,
                         uint8_t datalen1, uint8_t *data1, uint8_t tx_size,
                         uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer);
uint8_t atsha204_getSerialNumber(uint8_t *response);
uint8_t atsha204_read(uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t zone, uint16_t address);

#endif
#endif
