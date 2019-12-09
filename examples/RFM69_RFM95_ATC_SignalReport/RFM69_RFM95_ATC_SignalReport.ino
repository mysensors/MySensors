/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - tekka
 *
 * DESCRIPTION
 * ATC mode settings and signal report functions, on RFM69 and RFM95 nodes
 *
 */

// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached

// RFM69
//#define MY_RADIO_RFM69
//#define MY_RFM69_NEW_DRIVER   // ATC on RFM69 works only with the new driver (not compatible with old=default driver)
//#define MY_RFM69_ATC_TARGET_RSSI_DBM (-70)  // target RSSI -70dBm
//#define MY_RFM69_MAX_POWER_LEVEL_DBM (10)   // max. TX power 10dBm = 10mW

// RFM95
#define MY_RADIO_RFM95
#define MY_RFM95_ATC_TARGET_RSSI_DBM (-70)  // target RSSI -70dBm
#define MY_RFM95_MAX_POWER_LEVEL_DBM (10)   // max. TX power 10dBm = 10mW

#include <MySensors.h>

// ID of the sensor child
#define CHILD_ID_UPLINK_QUALITY (0)
#define CHILD_ID_TX_LEVEL       (1)
#define CHILD_ID_TX_PERCENT     (2)
#define CHILD_ID_TX_RSSI        (3)
#define CHILD_ID_RX_RSSI        (4)
#define CHILD_ID_TX_SNR         (5)
#define CHILD_ID_RX_SNR         (6)


// Initialize general message
MyMessage msgTxRSSI(CHILD_ID_TX_RSSI, V_CUSTOM);
MyMessage msgRxRSSI(CHILD_ID_RX_RSSI, V_CUSTOM);
MyMessage msgTxSNR(CHILD_ID_TX_SNR, V_CUSTOM);
MyMessage msgRxSNR(CHILD_ID_RX_SNR, V_CUSTOM);
MyMessage msgTxLevel(CHILD_ID_TX_LEVEL, V_CUSTOM);
MyMessage msgTxPercent(CHILD_ID_TX_PERCENT, V_CUSTOM);
MyMessage msgUplinkQuality(CHILD_ID_UPLINK_QUALITY, V_CUSTOM);

void setup()
{
}


void presentation()
{
	// Send the sketch version information to the gateway and controller
	sendSketchInfo("ATC", "1.0");

	// Register all sensors to gw (they will be created as child devices)
	present(CHILD_ID_UPLINK_QUALITY, S_CUSTOM, "UPLINK QUALITY RSSI");
	present(CHILD_ID_TX_LEVEL, S_CUSTOM, "TX LEVEL DBM");
	present(CHILD_ID_TX_PERCENT, S_CUSTOM, "TX LEVEL PERCENT");
	present(CHILD_ID_TX_RSSI, S_CUSTOM, "TX RSSI");
	present(CHILD_ID_RX_RSSI, S_CUSTOM, "RX RSSI");
	present(CHILD_ID_TX_SNR, S_CUSTOM, "TX SNR");
	present(CHILD_ID_RX_SNR, S_CUSTOM, "RX SNR");
}

void loop()
{
	// send messages to GW
	send(msgUplinkQuality.set(transportGetSignalReport(SR_UPLINK_QUALITY)));
	send(msgTxLevel.set(transportGetSignalReport(SR_TX_POWER_LEVEL)));
	send(msgTxPercent.set(transportGetSignalReport(SR_TX_POWER_PERCENT)));
	// retrieve RSSI / SNR reports from incoming ACK
	send(msgTxRSSI.set(transportGetSignalReport(SR_TX_RSSI)));
	send(msgRxRSSI.set(transportGetSignalReport(SR_RX_RSSI)));
	send(msgTxSNR.set(transportGetSignalReport(SR_TX_SNR)));
	send(msgRxSNR.set(transportGetSignalReport(SR_RX_SNR)));
	// wait a bit
	wait(5000);
}
