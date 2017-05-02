#include "Arduino.h"
#include "sha204_library.h"
#include "sha204_lib_return_codes.h"


// atsha204Class Constructor
// Feed this function the Arduino-ized pin number you want to assign to the ATSHA204's SDA pin
// For AVR ARCH, this will find the DDRX, PORTX, and PINX registrs it'll need to point to to control that pin
// As well as the bit value for each of those registers
// For others it will only store the pin number which is used in subsequent functions

atsha204Class::atsha204Class(uint8_t pin)
{
#if defined(ARDUINO_ARCH_AVR)
	device_pin = digitalPinToBitMask(pin);	// Find the bit value of the pin
	uint8_t port = digitalPinToPort(pin);	// temoporarily used to get the next three registers

	// Point to data direction register port of pin
	device_port_DDR = portModeRegister(port);
	// Point to output register of pin
	device_port_OUT = portOutputRegister(port);
	// Point to input register of pin
	device_port_IN = portInputRegister(port);
#else
	device_pin = pin;
#endif
}

/* 	Puts a the ATSHA204's unique, 4-byte serial number in the response array
	returns an SHA204 Return code */
uint8_t atsha204Class::getSerialNumber(uint8_t * response)
{
	uint8_t readCommand[READ_COUNT];
	uint8_t readResponse[READ_4_RSP_SIZE];

	/* read from bytes 0->3 of config zone */
	uint8_t returnCode = sha204m_read(readCommand, readResponse, SHA204_ZONE_CONFIG, ADDRESS_SN03);
	if (!returnCode) {	// should return 0 if successful
		for (int i=0; i<4; i++) {	// store bytes 0-3 into respones array
			response[i] = readResponse[SHA204_BUFFER_POS_DATA+i];
		}

		/* read from bytes 8->11 of config zone */
		returnCode = sha204m_read(readCommand, readResponse, SHA204_ZONE_CONFIG, ADDRESS_SN47);

		for (int i=4; i<8; i++) {	// store bytes 4-7 of SN into response array
			response[i] = readResponse[SHA204_BUFFER_POS_DATA+(i-4)];
		}

		if (!returnCode) {
			/* Finally if last two reads were successful, read byte 8 of the SN */
			returnCode = sha204m_read(readCommand, readResponse, SHA204_ZONE_CONFIG, ADDRESS_SN8);
			response[8] = readResponse[SHA204_BUFFER_POS_DATA];	// Byte 8 of SN should always be 0xEE
		}
	}

	return returnCode;
}

/*  Calculates CRC16 value of provided data (and optionally including provided existing CRC16 data)
  returns the calculated CRC16 value */
uint16_t atsha204Class::calculateAndUpdateCrc(uint8_t length, uint8_t *data, uint16_t current_crc)
{
	uint8_t counter;
	uint16_t crc_register = current_crc;
	uint16_t polynom = 0x8005;
	uint8_t shift_register;
	uint8_t data_bit, crc_bit;

	for (counter = 0; counter < length; counter++) {
		for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
			data_bit = (data[counter] & shift_register) ? 1 : 0;
			crc_bit = crc_register >> 15;

			// Shift CRC to the left by 1.
			crc_register <<= 1;

			if ((data_bit ^ crc_bit) != 0) {
				crc_register ^= polynom;
			}
		}
	}
	return crc_register;
}

/* SWI bit bang functions */

void atsha204Class::swi_set_signal_pin(uint8_t is_high)
{
	SHA204_SET_OUTPUT();
	if (is_high) {
		SHA204_POUT_HIGH();
	} else {
		SHA204_POUT_LOW();
	}

}

uint8_t atsha204Class::swi_send_bytes(uint8_t count, uint8_t *buffer)
{
	uint8_t i, bit_mask;

	// Disable interrupts while sending.
	noInterrupts();  //swi_disable_interrupts();

	// Set signal pin as output.
	SHA204_POUT_HIGH();
	SHA204_SET_OUTPUT();
	// Wait turn around time.
	delayMicroseconds(RX_TX_DELAY);  //RX_TX_DELAY;

	for (i = 0; i < count; i++) {
		for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1) {
			if (bit_mask & buffer[i]) {
				SHA204_POUT_LOW();
				delayMicroseconds(BIT_DELAY);  //BIT_DELAY_1;
				SHA204_POUT_HIGH();
				delayMicroseconds(7*BIT_DELAY);  //BIT_DELAY_7;
			} else {
				// Send a zero bit.
				SHA204_POUT_LOW();
				delayMicroseconds(BIT_DELAY);  //BIT_DELAY_1;
				SHA204_POUT_HIGH();
				delayMicroseconds(BIT_DELAY);  //BIT_DELAY_1;
				SHA204_POUT_LOW();
				delayMicroseconds(BIT_DELAY);  //BIT_DELAY_1;
				SHA204_POUT_HIGH();
				delayMicroseconds(5*BIT_DELAY);  //BIT_DELAY_5;
			}
		}
	}
	interrupts();  //swi_enable_interrupts();
	return SWI_FUNCTION_RETCODE_SUCCESS;
}

uint8_t atsha204Class::swi_send_byte(uint8_t value)
{
	return swi_send_bytes(1, &value);
}

uint8_t atsha204Class::swi_receive_bytes(uint8_t count, uint8_t *buffer)
{
	uint8_t status = SWI_FUNCTION_RETCODE_SUCCESS;
	uint8_t i;
	uint8_t bit_mask;
	uint8_t pulse_count;
	uint8_t timeout_count;

	// Disable interrupts while receiving.
	noInterrupts(); //swi_disable_interrupts();

	// Configure signal pin as input
	SHA204_SET_INPUT();

	// Receive bits and store in buffer.
	for (i = 0; i < count; i++) {
		for (bit_mask = 1; bit_mask > 0; bit_mask <<= 1) {
			pulse_count = 0;

			// Make sure that the variable below is big enough.
			// Change it to uint16_t if 255 is too small, but be aware that
			// the loop resolution decreases on an 8-bit controller in that case.
			timeout_count = START_PULSE_TIME_OUT;

			// Detect start bit.
			while (--timeout_count > 0) {
				// Wait for falling edge.
				if (SHA204_PIN_READ() == 0) {
					break;
				}
			}

			if (timeout_count == 0) {
				status = SWI_FUNCTION_RETCODE_TIMEOUT;
				break;
			}

			do {
				// Wait for rising edge.
				if (SHA204_PIN_READ() != 0) {
					// For an Atmel microcontroller this might be faster than "pulse_count++".
					pulse_count = 1;
					break;
				}
			} while (--timeout_count > 0);

			if (pulse_count == 0) {
				status = SWI_FUNCTION_RETCODE_TIMEOUT;
				break;
			}

			// Trying to measure the time of start bit and calculating the timeout
			// for zero bit detection is not accurate enough for an 8 MHz 8-bit CPU.
			// So let's just wait the maximum time for the falling edge of a zero bit
			// to arrive after we have detected the rising edge of the start bit.
			timeout_count = ZERO_PULSE_TIME_OUT;

			// Detect possible edge indicating zero bit.
			do {
				if (SHA204_PIN_READ() == 0) {
					// For an Atmel microcontroller this might be faster than "pulse_count++".
					pulse_count = 2;
					break;
				}
			} while (--timeout_count > 0);

			// Wait for rising edge of zero pulse before returning. Otherwise we might interpret
			// its rising edge as the next start pulse.
			if (pulse_count == 2) {
				do {
					if (SHA204_PIN_READ() != 0) {
						break;
					}
				} while (timeout_count-- > 0);
			}

			// Update byte at current buffer index.
			else {
				buffer[i] |= bit_mask;    // received "one" bit
			}
		}

		if (status != SWI_FUNCTION_RETCODE_SUCCESS) {
			break;
		}
	}
	interrupts(); //swi_enable_interrupts();

	if (status == SWI_FUNCTION_RETCODE_TIMEOUT) {
		if (i > 0)
			// Indicate that we timed out after having received at least one byte.
		{
			status = SWI_FUNCTION_RETCODE_RX_FAIL;
		}
	}
	return status;
}

/* Physical functions */

uint8_t atsha204Class::sha204p_wakeup()
{
	swi_set_signal_pin(0);
	delayMicroseconds(10*SHA204_WAKEUP_PULSE_WIDTH);
	swi_set_signal_pin(1);
	delay(SHA204_WAKEUP_DELAY);

	return SHA204_SUCCESS;
}

uint8_t atsha204Class::sha204p_sleep()
{
	return swi_send_byte(SHA204_SWI_FLAG_SLEEP);
}

uint8_t atsha204Class::sha204p_resync(uint8_t size, uint8_t *response)
{
	delay(SHA204_SYNC_TIMEOUT);
	return sha204p_receive_response(size, response);
}

uint8_t atsha204Class::sha204p_receive_response(uint8_t size, uint8_t *response)
{
	uint8_t i;
	uint8_t ret_code;

	for (i = 0; i < size; i++) {
		response[i] = 0;
	}

	(void) swi_send_byte(SHA204_SWI_FLAG_TX);

	ret_code = swi_receive_bytes(size, response);
	if (ret_code == SWI_FUNCTION_RETCODE_SUCCESS || ret_code == SWI_FUNCTION_RETCODE_RX_FAIL) {
		uint8_t count_byte;
		count_byte = response[SHA204_BUFFER_POS_COUNT];
		if ((count_byte < SHA204_RSP_SIZE_MIN) || (count_byte > size)) {
			return SHA204_INVALID_SIZE;
		}

		return SHA204_SUCCESS;
	}

	// Translate error so that the Communication layer
	// can distinguish between a real error or the
	// device being busy executing a command.
	if (ret_code == SWI_FUNCTION_RETCODE_TIMEOUT) {
		return SHA204_RX_NO_RESPONSE;
	} else {
		return SHA204_RX_FAIL;
	}
}

uint8_t atsha204Class::sha204p_send_command(uint8_t count, uint8_t * command)
{
	uint8_t ret_code = swi_send_byte(SHA204_SWI_FLAG_CMD);
	if (ret_code != SWI_FUNCTION_RETCODE_SUCCESS) {
		return SHA204_COMM_FAIL;
	}

	return swi_send_bytes(count, command);
}

/* Communication functions */

uint8_t atsha204Class::sha204c_wakeup(uint8_t *response)
{
	uint8_t ret_code = sha204p_wakeup();
	if (ret_code != SHA204_SUCCESS) {
		return ret_code;
	}

	ret_code = sha204p_receive_response(SHA204_RSP_SIZE_MIN, response);
	if (ret_code != SHA204_SUCCESS) {
		return ret_code;
	}

	// Verify status response.
	if (response[SHA204_BUFFER_POS_COUNT] != SHA204_RSP_SIZE_MIN) {
		ret_code = SHA204_INVALID_SIZE;
	} else if (response[SHA204_BUFFER_POS_STATUS] != SHA204_STATUS_BYTE_WAKEUP) {
		ret_code = SHA204_COMM_FAIL;
	} else {
		if ((response[SHA204_RSP_SIZE_MIN - SHA204_CRC_SIZE] != 0x33)
		        || (response[SHA204_RSP_SIZE_MIN + 1 - SHA204_CRC_SIZE] != 0x43)) {
			ret_code = SHA204_BAD_CRC;
		}
	}
	if (ret_code != SHA204_SUCCESS) {
		delay(SHA204_COMMAND_EXEC_MAX);
	}

	return ret_code;
}

uint8_t atsha204Class::sha204c_resync(uint8_t size, uint8_t *response)
{
	// Try to re-synchronize without sending a Wake token
	// (step 1 of the re-synchronization process).
	uint8_t ret_code = sha204p_resync(size, response);
	if (ret_code == SHA204_SUCCESS) {
		return ret_code;
	}

	// We lost communication. Send a Wake pulse and try
	// to receive a response (steps 2 and 3 of the
	// re-synchronization process).
	(void) sha204p_sleep();
	ret_code = sha204c_wakeup(response);

	// Translate a return value of success into one
	// that indicates that the device had to be woken up
	// and might have lost its TempKey.
	return (ret_code == SHA204_SUCCESS ? SHA204_RESYNC_WITH_WAKEUP : ret_code);
}

uint8_t atsha204Class::sha204c_send_and_receive(uint8_t *tx_buffer, uint8_t rx_size,
        uint8_t *rx_buffer, uint8_t execution_delay, uint8_t execution_timeout)
{
	uint8_t ret_code = SHA204_FUNC_FAIL;
	uint8_t ret_code_resync;
	uint8_t n_retries_send;
	uint8_t n_retries_receive;
	uint8_t i;
	uint8_t status_byte;
	uint8_t count = tx_buffer[SHA204_BUFFER_POS_COUNT];
	uint8_t count_minus_crc = count - SHA204_CRC_SIZE;
	uint16_t execution_timeout_us = (uint16_t) (execution_timeout * 1000) + SHA204_RESPONSE_TIMEOUT;
	volatile uint16_t timeout_countdown;

	// Append CRC.
	sha204c_calculate_crc(count_minus_crc, tx_buffer, tx_buffer + count_minus_crc);

	// Retry loop for sending a command and receiving a response.
	n_retries_send = SHA204_RETRY_COUNT + 1;

	while ((n_retries_send-- > 0) && (ret_code != SHA204_SUCCESS)) {
		// Send command.
		ret_code = sha204p_send_command(count, tx_buffer);
		if (ret_code != SHA204_SUCCESS) {
			if (sha204c_resync(rx_size, rx_buffer) == SHA204_RX_NO_RESPONSE) {
				return ret_code;    // The device seems to be dead in the water.
			} else {
				continue;
			}
		}

		// Wait minimum command execution time and then start polling for a response.
		delay(execution_delay);

		// Retry loop for receiving a response.
		n_retries_receive = SHA204_RETRY_COUNT + 1;
		while (n_retries_receive-- > 0) {
			// Reset response buffer.
			for (i = 0; i < rx_size; i++) {
				rx_buffer[i] = 0;
			}

			// Poll for response.
			timeout_countdown = execution_timeout_us;
			do {
				ret_code = sha204p_receive_response(rx_size, rx_buffer);
				timeout_countdown -= SHA204_RESPONSE_TIMEOUT;
			} while ((timeout_countdown > SHA204_RESPONSE_TIMEOUT) && (ret_code == SHA204_RX_NO_RESPONSE));

			if (ret_code == SHA204_RX_NO_RESPONSE) {
				// We did not receive a response. Re-synchronize and send command again.
				if (sha204c_resync(rx_size, rx_buffer) == SHA204_RX_NO_RESPONSE)
					// The device seems to be dead in the water.
				{
					return ret_code;
				} else {
					break;
				}
			}

			// Check whether we received a valid response.
			if (ret_code == SHA204_INVALID_SIZE) {
				// We see 0xFF for the count when communication got out of sync.
				ret_code_resync = sha204c_resync(rx_size, rx_buffer);
				if (ret_code_resync == SHA204_SUCCESS)
					// We did not have to wake up the device. Try receiving response again.
				{
					continue;
				}
				if (ret_code_resync == SHA204_RESYNC_WITH_WAKEUP)
					// We could re-synchronize, but only after waking up the device.
					// Re-send command.
				{
					break;
				} else
					// We failed to re-synchronize.
				{
					return ret_code;
				}
			}

			// We received a response of valid size.
			// Check the consistency of the response.
			ret_code = sha204c_check_crc(rx_buffer);
			if (ret_code == SHA204_SUCCESS) {
				// Received valid response.
				if (rx_buffer[SHA204_BUFFER_POS_COUNT] > SHA204_RSP_SIZE_MIN)
					// Received non-status response. We are done.
				{
					return ret_code;
				}

				// Received status response.
				status_byte = rx_buffer[SHA204_BUFFER_POS_STATUS];

				// Translate the three possible device status error codes
				// into library return codes.
				if (status_byte == SHA204_STATUS_BYTE_PARSE) {
					return SHA204_PARSE_ERROR;
				}
				if (status_byte == SHA204_STATUS_BYTE_EXEC) {
					return SHA204_CMD_FAIL;
				}
				if (status_byte == SHA204_STATUS_BYTE_COMM) {
					// In case of the device status byte indicating a communication
					// error this function exits the retry loop for receiving a response
					// and enters the overall retry loop
					// (send command / receive response).
					ret_code = SHA204_STATUS_CRC;
					break;
				}

				// Received status response from CheckMAC, DeriveKey, GenDig,
				// Lock, Nonce, Pause, UpdateExtra, or Write command.
				return ret_code;
			}

			else {
				// Received response with incorrect CRC.
				ret_code_resync = sha204c_resync(rx_size, rx_buffer);
				if (ret_code_resync == SHA204_SUCCESS)
					// We did not have to wake up the device. Try receiving response again.
				{
					continue;
				}
				if (ret_code_resync == SHA204_RESYNC_WITH_WAKEUP)
					// We could re-synchronize, but only after waking up the device.
					// Re-send command.
				{
					break;
				} else
					// We failed to re-synchronize.
				{
					return ret_code;
				}
			} // block end of check response consistency

		} // block end of receive retry loop

	} // block end of send and receive retry loop

	return ret_code;
}


/* Marshaling functions */

uint8_t atsha204Class::sha204m_random(uint8_t * tx_buffer, uint8_t * rx_buffer, uint8_t mode)
{
	if (!tx_buffer || !rx_buffer || (mode > RANDOM_NO_SEED_UPDATE)) {
		return SHA204_BAD_PARAM;
	}

	tx_buffer[SHA204_COUNT_IDX] = RANDOM_COUNT;
	tx_buffer[SHA204_OPCODE_IDX] = SHA204_RANDOM;
	tx_buffer[RANDOM_MODE_IDX] = mode & RANDOM_SEED_UPDATE;

	tx_buffer[RANDOM_PARAM2_IDX] =
	    tx_buffer[RANDOM_PARAM2_IDX + 1] = 0;

	return sha204c_send_and_receive(&tx_buffer[0], RANDOM_RSP_SIZE, &rx_buffer[0], RANDOM_DELAY,
	                                RANDOM_EXEC_MAX - RANDOM_DELAY);
}

uint8_t atsha204Class::sha204m_dev_rev(uint8_t *tx_buffer, uint8_t *rx_buffer)
{
	if (!tx_buffer || !rx_buffer) {
		return SHA204_BAD_PARAM;
	}

	tx_buffer[SHA204_COUNT_IDX] = DEVREV_COUNT;
	tx_buffer[SHA204_OPCODE_IDX] = SHA204_DEVREV;

	// Parameters are 0.
	tx_buffer[DEVREV_PARAM1_IDX] =
	    tx_buffer[DEVREV_PARAM2_IDX] =
	        tx_buffer[DEVREV_PARAM2_IDX + 1] = 0;

	return sha204c_send_and_receive(&tx_buffer[0], DEVREV_RSP_SIZE, &rx_buffer[0],
	                                DEVREV_DELAY, DEVREV_EXEC_MAX - DEVREV_DELAY);
}

uint8_t atsha204Class::sha204m_read(uint8_t *tx_buffer, uint8_t *rx_buffer, uint8_t zone,
                                    uint16_t address)
{
	uint8_t rx_size;

	if (!tx_buffer || !rx_buffer || ((zone & ~READ_ZONE_MASK) != 0)
	        || ((zone & READ_ZONE_MODE_32_BYTES) && (zone == SHA204_ZONE_OTP))) {
		return SHA204_BAD_PARAM;
	}

	address >>= 2;
	if ((zone & SHA204_ZONE_MASK) == SHA204_ZONE_CONFIG) {
		if (address > SHA204_ADDRESS_MASK_CONFIG) {
			return SHA204_BAD_PARAM;
		}
	} else if ((zone & SHA204_ZONE_MASK) == SHA204_ZONE_OTP) {
		if (address > SHA204_ADDRESS_MASK_OTP) {
			return SHA204_BAD_PARAM;
		}
	} else if ((zone & SHA204_ZONE_MASK) == SHA204_ZONE_DATA) {
		if (address > SHA204_ADDRESS_MASK) {
			return SHA204_BAD_PARAM;
		}
	}

	tx_buffer[SHA204_COUNT_IDX] = READ_COUNT;
	tx_buffer[SHA204_OPCODE_IDX] = SHA204_READ;
	tx_buffer[READ_ZONE_IDX] = zone;
	tx_buffer[READ_ADDR_IDX] = (uint8_t) (address & SHA204_ADDRESS_MASK);
	tx_buffer[READ_ADDR_IDX + 1] = 0;

	rx_size = (zone & SHA204_ZONE_COUNT_FLAG) ? READ_32_RSP_SIZE : READ_4_RSP_SIZE;

	return sha204c_send_and_receive(&tx_buffer[0], rx_size, &rx_buffer[0], READ_DELAY,
	                                READ_EXEC_MAX - READ_DELAY);
}

uint8_t atsha204Class::sha204m_execute(uint8_t op_code, uint8_t param1, uint16_t param2,
                                       uint8_t datalen1, uint8_t *data1, uint8_t datalen2, uint8_t *data2, uint8_t datalen3,
                                       uint8_t *data3,
                                       uint8_t tx_size, uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer)
{
	uint8_t poll_delay, poll_timeout, response_size;
	uint8_t *p_buffer;
	uint8_t len;

	uint8_t ret_code = sha204m_check_parameters(op_code, param1, param2,
	                   datalen1, data1, datalen2, data2, datalen3, data3,
	                   tx_size, tx_buffer, rx_size, rx_buffer);
	if (ret_code != SHA204_SUCCESS) {
		return ret_code;
	}

	// Supply delays and response size.
	switch (op_code) {
	case SHA204_CHECKMAC:
		poll_delay = CHECKMAC_DELAY;
		poll_timeout = CHECKMAC_EXEC_MAX - CHECKMAC_DELAY;
		response_size = CHECKMAC_RSP_SIZE;
		break;

	case SHA204_DERIVE_KEY:
		poll_delay = DERIVE_KEY_DELAY;
		poll_timeout = DERIVE_KEY_EXEC_MAX - DERIVE_KEY_DELAY;
		response_size = DERIVE_KEY_RSP_SIZE;
		break;

	case SHA204_DEVREV:
		poll_delay = DEVREV_DELAY;
		poll_timeout = DEVREV_EXEC_MAX - DEVREV_DELAY;
		response_size = DEVREV_RSP_SIZE;
		break;

	case SHA204_GENDIG:
		poll_delay = GENDIG_DELAY;
		poll_timeout = GENDIG_EXEC_MAX - GENDIG_DELAY;
		response_size = GENDIG_RSP_SIZE;
		break;

	case SHA204_HMAC:
		poll_delay = HMAC_DELAY;
		poll_timeout = HMAC_EXEC_MAX - HMAC_DELAY;
		response_size = HMAC_RSP_SIZE;
		break;

	case SHA204_LOCK:
		poll_delay = LOCK_DELAY;
		poll_timeout = LOCK_EXEC_MAX - LOCK_DELAY;
		response_size = LOCK_RSP_SIZE;
		break;

	case SHA204_MAC:
		poll_delay = MAC_DELAY;
		poll_timeout = MAC_EXEC_MAX - MAC_DELAY;
		response_size = MAC_RSP_SIZE;
		break;

	case SHA204_NONCE:
		poll_delay = NONCE_DELAY;
		poll_timeout = NONCE_EXEC_MAX - NONCE_DELAY;
		response_size = param1 == NONCE_MODE_PASSTHROUGH
		                ? NONCE_RSP_SIZE_SHORT : NONCE_RSP_SIZE_LONG;
		break;

	case SHA204_PAUSE:
		poll_delay = PAUSE_DELAY;
		poll_timeout = PAUSE_EXEC_MAX - PAUSE_DELAY;
		response_size = PAUSE_RSP_SIZE;
		break;

	case SHA204_RANDOM:
		poll_delay = RANDOM_DELAY;
		poll_timeout = RANDOM_EXEC_MAX - RANDOM_DELAY;
		response_size = RANDOM_RSP_SIZE;
		break;

	case SHA204_READ:
		poll_delay = READ_DELAY;
		poll_timeout = READ_EXEC_MAX - READ_DELAY;
		response_size = (param1 & SHA204_ZONE_COUNT_FLAG)
		                ? READ_32_RSP_SIZE : READ_4_RSP_SIZE;
		break;

	case SHA204_UPDATE_EXTRA:
		poll_delay = UPDATE_DELAY;
		poll_timeout = UPDATE_EXEC_MAX - UPDATE_DELAY;
		response_size = UPDATE_RSP_SIZE;
		break;

	case SHA204_WRITE:
		poll_delay = WRITE_DELAY;
		poll_timeout = WRITE_EXEC_MAX - WRITE_DELAY;
		response_size = WRITE_RSP_SIZE;
		break;

	default:
		poll_delay = 0;
		poll_timeout = SHA204_COMMAND_EXEC_MAX;
		response_size = rx_size;
	}

	// Assemble command.
	len = datalen1 + datalen2 + datalen3 + SHA204_CMD_SIZE_MIN;
	p_buffer = tx_buffer;
	*p_buffer++ = len;
	*p_buffer++ = op_code;
	*p_buffer++ = param1;
	*p_buffer++ = param2 & 0xFF;
	*p_buffer++ = param2 >> 8;

	if (datalen1 > 0) {
		memcpy(p_buffer, data1, datalen1);
		p_buffer += datalen1;
	}
	if (datalen2 > 0) {
		memcpy(p_buffer, data2, datalen2);
		p_buffer += datalen2;
	}
	if (datalen3 > 0) {
		memcpy(p_buffer, data3, datalen3);
		p_buffer += datalen3;
	}

	sha204c_calculate_crc(len - SHA204_CRC_SIZE, tx_buffer, p_buffer);

	// Send command and receive response.
	return sha204c_send_and_receive(&tx_buffer[0], response_size,
	                                &rx_buffer[0],	poll_delay, poll_timeout);
}

uint8_t atsha204Class::sha204m_check_parameters(uint8_t op_code, uint8_t param1, uint16_t param2,
        uint8_t datalen1, uint8_t *data1, uint8_t datalen2, uint8_t *data2, uint8_t datalen3,
        uint8_t *data3,
        uint8_t tx_size, uint8_t *tx_buffer, uint8_t rx_size, uint8_t *rx_buffer)
{
#ifdef SHA204_CHECK_PARAMETERS

	uint8_t len = datalen1 + datalen2 + datalen3 + SHA204_CMD_SIZE_MIN;
	if (!tx_buffer || tx_size < len || rx_size < SHA204_RSP_SIZE_MIN || !rx_buffer) {
		return SHA204_BAD_PARAM;
	}

	if ((datalen1 > 0 && !data1) || (datalen2 > 0 && !data2) || (datalen3 > 0 && !data3)) {
		return SHA204_BAD_PARAM;
	}

	// Check parameters depending on op-code.
	switch (op_code) {
	case SHA204_CHECKMAC:
		if (
		    // no null pointers allowed
		    !data1 || !data2
		    // No reserved bits should be set.
		    || (param1 | CHECKMAC_MODE_MASK) != CHECKMAC_MODE_MASK
		    // key_id > 15 not allowed
		    || param2 > SHA204_KEY_ID_MAX
		) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_DERIVE_KEY:
		if (param2 > SHA204_KEY_ID_MAX) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_DEVREV:
		break;

	case SHA204_GENDIG:
		if ((param1 != GENDIG_ZONE_OTP) && (param1 != GENDIG_ZONE_DATA)) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_HMAC:
		if ((param1 & ~HMAC_MODE_MASK) != 0) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_LOCK:
		if (((param1 & ~LOCK_ZONE_MASK) != 0)
		        || ((param1 & LOCK_ZONE_NO_CRC) && (param2 != 0))) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_MAC:
		if (((param1 & ~MAC_MODE_MASK) != 0)
		        || (((param1 & MAC_MODE_BLOCK2_TEMPKEY) == 0) && !data1)) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_NONCE:
		if (  !data1
		        || (param1 > NONCE_MODE_PASSTHROUGH)
		        || (param1 == NONCE_MODE_INVALID)
		   ) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_PAUSE:
		break;

	case SHA204_RANDOM:
		if (param1 > RANDOM_NO_SEED_UPDATE) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_READ:
		if (((param1 & ~READ_ZONE_MASK) != 0)
		        || ((param1 & READ_ZONE_MODE_32_BYTES) && (param1 == SHA204_ZONE_OTP))) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_TEMPSENSE:
		break;

	case SHA204_UPDATE_EXTRA:
		if (param1 > UPDATE_CONFIG_BYTE_86) {
			return SHA204_BAD_PARAM;
		}
		break;

	case SHA204_WRITE:
		if (!data1 || ((param1 & ~WRITE_ZONE_MASK) != 0)) {
			return SHA204_BAD_PARAM;
		}
		break;

	default:
		// unknown op-code
		return SHA204_BAD_PARAM;
	}

	return SHA204_SUCCESS;

#else
	(void)rx_size;
	(void)tx_size;
	(void)tx_buffer;
	(void)rx_buffer;
	(void)param1;
	(void)param2;
	(void)data1;
	(void)data2;
	(void)data3;
	(void)datalen1;
	(void)datalen2;
	(void)datalen3;
	(void)op_code;
	return SHA204_SUCCESS;
#endif
}

/* CRC Calculator and Checker */

void atsha204Class::sha204c_calculate_crc(uint8_t length, uint8_t *data, uint8_t *crc)
{
	uint16_t crc_register = 0;

	crc_register = calculateAndUpdateCrc(length, data, crc_register);
	crc[0] = (uint8_t) (crc_register & 0x00FF);
	crc[1] = (uint8_t) (crc_register >> 8);
}

uint8_t atsha204Class::sha204c_check_crc(uint8_t *response)
{
	uint8_t crc[SHA204_CRC_SIZE];
	uint8_t count = response[SHA204_BUFFER_POS_COUNT];

	count -= SHA204_CRC_SIZE;
	sha204c_calculate_crc(count, response, crc);

	return (crc[0] == response[count] && crc[1] == response[count + 1])
	       ? SHA204_SUCCESS : SHA204_BAD_CRC;
}
