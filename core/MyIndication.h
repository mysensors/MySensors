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
 */

#ifndef MyIndication_h
#define MyIndication_h

/**
 * Indication type
 */
typedef enum {
	INDICATION_TX = 0,                        //!< Sent a message.
	INDICATION_RX,                            //!< Received a message.

	INDICATION_GW_TX,                         //!< Gateway transmit message.
	INDICATION_GW_RX,                         //!< Gateway receive message.

	INDICATION_FIND_PARENT,                   //!< Start finding parent node.
	INDICATION_GOT_PARENT,                    //!< Found parent node.
	INDICATION_REQ_NODEID,                    //!< Request node ID.
	INDICATION_GOT_NODEID,                    //!< Got a node ID.
	INDICATION_CHECK_UPLINK,                  //!< Check uplink
	INDICATION_REQ_REGISTRATION,              //!< Request node registration.
	INDICATION_GOT_REGISTRATION,              //!< Got registration response.
	INDICATION_REBOOT,                        //!< Rebooting node.
	INDICATION_PRESENT,                       //!< Presenting node to gateway.
	INDICATION_CLEAR_ROUTING,                 //!< Clear routing table requested.
	INDICATION_SLEEP,                         //!< Node goes to sleep.
	INDICATION_WAKEUP,                        //!< Node just woke from sleep.
	INDICATION_FW_UPDATE_START,               //!< Start of OTA firmware update process.
	INDICATION_FW_UPDATE_RX,                  //!< Received a piece of firmware data.
	INDICATION_FW_UPDATE_RX_ERR,              //!< Received wrong piece of firmware data.

	INDICATION_ERR_START = 100,
	INDICATION_ERR_HW_INIT,                   //!< HW initialization error
	INDICATION_ERR_TX,                        //!< Failed to transmit message.
	INDICATION_ERR_TRANSPORT_FAILURE,         //!< Transport failure.
	INDICATION_ERR_INIT_TRANSPORT,            //!< MySensors transport hardware (radio) init failure.
	INDICATION_ERR_FIND_PARENT,               //!< Failed to find parent node.
	INDICATION_ERR_GET_NODEID,                //!< Failed to receive node ID.
	INDICATION_ERR_CHECK_UPLINK,              //!< Failed to check uplink
	INDICATION_ERR_SIGN,                      //!< Error signing.
	INDICATION_ERR_LENGTH,                    //!< Invalid message length.
	INDICATION_ERR_VERSION,                   //!< Protocol version mismatch.
	INDICATION_ERR_NET_FULL,                  //!< Network full. All node ID's are taken.
	INDICATION_ERR_INIT_GWTRANSPORT,          //!< Gateway transport hardware init failure.
	INDICATION_ERR_LOCKED,                    //!< Node is locked.
	INDICATION_ERR_FW_FLASH_INIT,             //!< Firmware update flash initialisation failure.
	INDICATION_ERR_FW_TIMEOUT,                //!< Firmware update timeout.
	INDICATION_ERR_FW_CHECKSUM,               //!< Firmware update checksum mismatch.
	INDICATION_ERR_END
} indication_t;

/**
 * Function which is called when something changes about the internal state of MySensors.
 * @param ind  Event indication of what happened.
 */
void setIndication( const indication_t ind );

/**
 * Allow user to define their own indication handler.
 */
void indication( const indication_t );

#endif
