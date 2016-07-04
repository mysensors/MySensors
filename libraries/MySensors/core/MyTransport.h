/*
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

 /**
 * @file MyTransport.h
 *
 * @brief API declaration for MyTransport
 */
#ifndef MyTransport_h
#define MyTransport_h

#include "MySensorsCore.h"

#if defined(MY_REPEATER_FEATURE)
	#define TRANSMISSION_FAILURES  10		//!< search for a new parent node after this many transmission failures, higher threshold for repeating nodes
#else
	#define TRANSMISSION_FAILURES  5		//!< search for a new parent node after this many transmission failures, lower threshold for non-repeating nodes
#endif
#define TIMEOUT_FAILURE_STATE 10000			//!< duration failure state
#define STATE_TIMEOUT 2000					//!< general state timeout
#define STATE_RETRIES 3						//!< retries before switching to FAILURE
#define AUTO 255							//!< ID 255 is reserved for auto initialization of nodeId.
#define BROADCAST_ADDRESS ((uint8_t)255)	//!< broadcasts are addressed to ID 255
#define DISTANCE_INVALID ((uint8_t)255)		//!< invalid distance when searching for parent
#define MAX_HOPS ((uint8_t)254)				//!< maximal mumber of hops for ping/pong
#define INVALID_HOPS ((uint8_t)255)			//!< invalid hops
#define MAX_SUBSEQ_MSGS 5					//!< Maximum number of subsequentially processed messages in FIFO (to prevent transport deadlock if HW issue)
#define CHKUPL_INTERVAL ((uint32_t)10000)	//!< Minimum time interval to re-check uplink

#define _autoFindParent (bool)(MY_PARENT_NODE_ID == AUTO)				//!<  returns true if static parent id is undefined
#define isValidDistance(distance) (bool)(distance!=DISTANCE_INVALID)	//!<  returns true if distance is valid
#define isValidParent(parent) (bool)(parent != AUTO)					//!<  returns true if parent is valid

 /**
 * @brief SM state
 *
 * This structure stores SM state definitions
 */
struct State {
	void(*Transition)();					//!< state transition function
	void(*Update)();						//!< state update function
};

/**
* @brief Status variables and SM state
*
* This structure stores transport status and SM variables
*/ 
typedef struct {
	State* currentState;					//!< pointer to current fsm state
	uint32_t stateEnter;					//!< state enter timepoint
	bool findingParentNode : 1;				//!< flag finding parent node is active
	bool preferredParentFound : 1;			//!< flag preferred parent found
	bool uplinkOk : 1;						//!< flag uplink ok
	bool pingActive : 1;					//!< flag ping active
	bool transportActive : 1;				//!< flag transport active
	uint8_t reserved : 3;					//!< reserved
	uint8_t retries : 4;					//!< retries / state re-enter
	uint8_t failedUplinkTransmissions : 4;	//!< counter failed uplink transmissions
	uint8_t pingResponse;					//!< stores hops received in I_PONG
} __attribute__((packed)) transportSM;


// PRIVATE functions

/**
* @brief Initialize SM variables and transport HW
*/
void stInitTransition();
/**
* @brief Find parent
*/
void stParentTransition();
/**
* @brief Verify find parent responses
*/
void stParentUpdate();
/**
* @brief Send ID request
*/
void stIDTransition();
/**
* @brief Verify ID response and GW link
*/
void stIDUpdate();
/**
* @brief Send uplink ping request
*/
void stUplinkTransition();
/**
* @brief Set transport OK
*/
void stOKTransition();
/**
* @brief Monitor transport link
*/
void stOKUpdate();
/**
* @brief Transport failure and power down radio 
*/
void stFailureTransition();
/**
* @brief Re-initialize transport after timeout
*/
void stFailureUpdate();
/**
* @brief Switch SM state
* @param newState New state to switch SM to
*/
void transportSwitchSM(State& newState);
/**
* @brief Update SM state
*/
void transportUpdateSM();
/**
* @brief Request time in current SM state
* @return ms in current state
*/
uint32_t transportTimeInState();

/**
* @brief Process all pending messages in RX FIFO
*/
void transportProcessFIFO();
/**
* @brief Receive message from RX FIFO and process
*/
void transportProcessMessage();
/**
* @brief Assign node ID
* @param newNodeId New node ID
*/
void transportAssignNodeID(uint8_t newNodeId);
/**
* @brief Wait and process messages for a defined amount of time until specified message received
* @param ms Time to wait and process incoming messages in ms
* @param cmd Specific command
* @param msgtype Specific message type 
* @return true if specified command received within waiting time
*/
bool transportWait(uint32_t ms, uint8_t cmd, uint8_t msgtype);
/**
* @brief Ping node
* @param targetId Node to be pinged
* @return hops from pinged node or 255 if no answer received within 2000ms
*/
uint8_t transportPingNode(uint8_t targetId);
/**
* @brief Send and route message according to destination
* 
* This function is used in MyTransport and omits the transport state check, i.e. message can be sent even if transport is !OK
*
* @param message
* @return true if message sent successfully
*/
bool transportRouteMessage(MyMessage &message);
/**
* @brief Send and route message according to destination with transport state check
* @param message
* @return true if message sent successfully and false if sending error or transport !OK
*/
bool transportSendRoute(MyMessage &message);
/**
* @brief Send message to recipient
* @param to Recipient of message
* @param message
* @return true if message sent successfully 
*/
bool transportSendWrite(uint8_t to, MyMessage &message);
/**
* @brief Check uplink to GW, includes flooding control
* @param force to override flood control timer
* @return true if uplink ok
*/
bool transportCheckUplink(bool force);

// PUBLIC functions

/**
* @brief Initialize transport and SM
*/
void transportInitialize();
/**
* @brief Process FIFO msg and update SM
*/
void transportProcess();
/**
* @brief Verify transport status
* @return true if transport is initialize and ready
*/
bool isTransportOK();
/**
* @brief Clear routing table
*/
void transportClearRoutingTable();
/**
* @brief Return heart beat, i.e. ms in current state
*/
uint32_t transportGetHeartbeat();

// interface functions for radio driver

/**
* @brief Initialize transport HW
* @return true if initalization successful
*/
bool transportInit();
/**
* @brief Set node address
*/
void transportSetAddress(uint8_t address);
/**
* @brief Retrieve node address
*/
uint8_t transportGetAddress();
/**
* @brief Send message
* @param to recipient
* @param data message to be sent
* @param len length of message (header + payload)
* @return true if message sent successfully
*/
bool transportSend(uint8_t to, const void* data, uint8_t len);
/**
* @brief Verify if RX FIFO has pending messages
* @return true if message available in RX FIFO
*/
bool transportAvailable();
/**
* @brief Sanity check for transport: is transport still responsive?
* @return true transport ok
*/
bool transportSanityCheck();
/**
* @brief Receive message from FIFO
* @return length of recevied message (header + payload)
*/
uint8_t transportReceive(void* data); 
/**
* @brief Power down transport HW
*/
void transportPowerDown();


#endif // MyTransport_h
/** @}*/
