/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2025 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * @file MyMainSTM32.cpp
 * @brief Main entry point implementation for STM32
 *
 * This file integrates with the Arduino framework's main() function.
 * The STM32duino core provides its own main() that calls setup() and loop().
 */

#include "MyHwSTM32.h"

/**
 * @file MyMainSTM32.cpp
 * @brief Main entry point for STM32
 *
 * STM32duino core provides main() function and serialEvent() handlers.
 * No additional implementation needed - the framework handles setup()/loop() calls.
 */

// Note: STM32duino core already provides weak serialEvent handlers
// We don't need to redefine them here
