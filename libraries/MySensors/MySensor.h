/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */


#ifndef MySensor_h
#define MySensor_h

#include "MyConfig.h"

// Include drivers
#include "drivers/SPI/SPI.cpp"

#include "drivers/RF24/RF24.cpp"

#include "drivers/RFM69/RFM69.cpp"

#include "drivers/ATSHA204/ATSHA204.cpp"
#include "drivers/ATSHA204/sha256.cpp"

#include "drivers/SPIFlash/SPIFlash.cpp"

#include "drivers/Ethernet_W5100/utility/socket.cpp"
#include "drivers/Ethernet_W5100/utility/w5100.cpp"
#include "drivers/Ethernet_W5100/DNS.cpp"
#include "drivers/Ethernet_W5100/Ethernet.cpp"
#include "drivers/Ethernet_W5100/EthernetUdp.cpp"
#include "drivers/Ethernet_W5100/IPAddress.cpp"



// Include core classes
#include "core/MyHw.cpp"
#include "core/MyHwATMega328.cpp"
#include "core/MyTransport.cpp"
#include "core/MyTransportNRF24.cpp"
#include "core/MyTransportRFM69.cpp"

#include "core/MyGatewayTransport.cpp"
#include "core/MyGatewayTransportSerial.cpp"
#include "core/MyMessage.cpp"
#include "core/MySensorCore.cpp"


// Initialize library
MyTransportRFM69 transport;
MySensor gw;


#endif
