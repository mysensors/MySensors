/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  Copyright (c) 2016 Sandeep Mistry All right reserved.
  Copyright (c) 2017 Frank Holtz All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "nrf.h"

#include "Arduino.h"
#include "nrf5_wiring_constants.h"

#ifdef __cplusplus
extern "C" {
#endif
void nrf5_pinMode(uint32_t ulPin, uint32_t ulMode)
{
	if (ulPin >= PINS_COUNT) {
		return;
	}

#ifdef ARDUINO_ARCH_NRF52
	// Arduino: https://github.com/arduino-org/arduino-core-nrf52
	ulPin = g_APinDescription[ulPin].ulPin;
#else
	// Sandeep Mistry: https://github.com/sandeepmistry/arduino-nRF5
	ulPin = g_ADigitalPinMap[ulPin];
#endif

	// Set pin mode according to chapter '22.6.3 I/O Pin Configuration'
	switch (ulMode) {
	case INPUT:
		// Set pin to input mode
		NRF_GPIO->PIN_CNF[ulPin] =
		    ((uint32_t)GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
		break;

	case INPUT_PULLUP:
		// Set pin to input mode with pull-up resistor enabled
		NRF_GPIO->PIN_CNF[ulPin] =
		    ((uint32_t)GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_PULL_Pullup << GPIO_PIN_CNF_PULL_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
		break;

	case INPUT_PULLDOWN:
		// Set pin to input mode with pull-down resistor enabled
		NRF_GPIO->PIN_CNF[ulPin] =
		    ((uint32_t)GPIO_PIN_CNF_DIR_Input << GPIO_PIN_CNF_DIR_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_INPUT_Connect << GPIO_PIN_CNF_INPUT_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_PULL_Pulldown << GPIO_PIN_CNF_PULL_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
		break;

	case OUTPUT:
		// Set pin to output mode
		NRF_GPIO->PIN_CNF[ulPin] =
		    ((uint32_t)GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_DRIVE_S0S1 << GPIO_PIN_CNF_DRIVE_Pos) |
		    ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
		break;

	default:
		// calculate nRF specific output modes
		if ((ulMode >= OUTPUT_S0S1) && (ulMode <= OUTPUT_H0D1)) {
			// Set pin to given output mode
			NRF_GPIO->PIN_CNF[ulPin] =
			    ((uint32_t)GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) |
			    ((uint32_t)GPIO_PIN_CNF_INPUT_Disconnect << GPIO_PIN_CNF_INPUT_Pos) |
			    ((uint32_t)GPIO_PIN_CNF_PULL_Disabled << GPIO_PIN_CNF_PULL_Pos) |
			    ((uint32_t)(ulMode - OUTPUT_S0S1) << GPIO_PIN_CNF_DRIVE_Pos) |
			    ((uint32_t)GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos);
		}
		break;
	}
}

#ifdef __cplusplus
}
#endif
