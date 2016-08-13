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
 * @defgroup MyTransportgrp MyTransport
 * @ingroup internals
 * @{
 *
 * Transport-related log messages, format: [!]SYSTEM:[SUB SYSTEM:]MESSAGE
 * - [!] Exclamation mark is prepended in case of error
 * - SYSTEM:
 *  - <b>TSM</b>: messages emitted by the transport state machine
 *  - <b>TSF</b>: messages emitted by transport support functions
 * - SUB SYSTEMS:
 *  - Transport state machine (<b>TSM</b>)
 *   - TSM:INIT						from <b>stInit</b> Initialize transport and radio
 *   - TSM:FPAR						from <b>stParent</b> Find parent
 *   - TSM:ID						from <b>stID</b> Check/request node ID, if dynamic node ID set
 *   - TSM:UPL						from <b>stUplink</b> Verify uplink connection by pinging GW
 *   - TSM:READY					from <b>stReady</b> Transport is ready and fully operational
 *   - TSM:FAILURE					from <b>stFailure</b> Failure in transport link or transport HW
 *  - Transport support function (<b>TSF</b>)
 *   - TSF:CHKUPL					from @ref transportCheckUplink(), checks connection to GW
 *   - TSF:ASID						from @ref transportAssignNodeID(), assigns node ID
 *   - TSF:PING						from @ref transportPingNode(), pings a node
 *   - TSF:CRT						from @ref transportClearRoutingTable(), clears routing table stored in EEPROM
 *   - TSF:MSG						from @ref transportProcessMessage(), processes incoming message
 *   - TSF:SANCHK					from @ref transportInvokeSanityCheck(), calls transport-specific sanity check
 *   - TSF:ROUTE					from @ref transportRouteMessage(), sends message
 *   - TSF:SEND						from @ref transportSendRoute(), sends message if transport is ready (exposed)
 *
 * Transport debug log messages:
 *
 * |E| SYS	| SUB		| Message				| Comment
 * |-|------|-----------|-----------------------|---------------------------------------------------------------------
 * | | TSM	| INIT		|						| <b>Transition to stInit state</b>
 * | | TSM	| INIT		| STATID,ID=%%d			| Node ID is static
 * | | TSM	| INIT		| TSP OK				| Transport device configured and fully operational
 * | | TSM	| INIT		| GW MODE				| Node is set up as GW, thus omitting ID and findParent states
 * |!| TSM	| INIT		| TSP FAIL				| Transport device initialization failed
 * | | TSM	| FPAR		|						| <b>Transition to stParent state</b>
 * | | TSM	| FPAR		| STATP=%%d				| Static parent set, skip finding parent
 * | | TSM	| FPAR		| OK					| Parent node identified
 * |!| TSM	| FPAR		| NO REPLY				| No potential parents replied to find parent request
 * |!| TSM	| FPAR		| FAIL					| Finding parent failed
 * | | TSM	| ID		|						| <b>Transition to stID state</b>
 * | | TSM	| ID		| OK,ID=%%d				| Node ID is valid
 * | | TSM	| ID		| REQ					| Request node ID from controller
 * |!| TSM	| ID		| FAIL,ID=%%d			| ID verification failed, ID invalid
 * | | TSM	| UPL		|						| <b>Transition to stUplink state</b>
 * | | TSM	| UPL		| OK					| Uplink OK, GW returned ping
 * |!| TSM	| UPL		| FAIL					| Uplink check failed, i.e. GW could not be pinged
 * | | TSM	| READY		|						| <b>Transition to stReady</b>, i.e. transport is ready and fully operational
 * |!| TSM	| READY		| UPL FAIL,SNP			| Too many failed uplink transmissions, search new parent
 * |!| TSM	| READY		| FAIL,STATP			| Too many failed uplink transmissions, static parent enforced
 * | | TSM	| FAILURE	|						| <b>Transition to stFailure state</b>
 * | | TSM	| FAILURE	| PDT					| Power-down transport
 * | | TSM	| FAILURE	| RE-INIT				| Attempt to re-initialize transport
 * | | TSF	| CHKUPL	| OK					| Uplink OK
 * | | TSF	| CHKUPL	| OK,FCTRL				| Uplink OK, flood control prevents pinging GW in too short intervals
 * | | TSF	| CHKUPL	| DGWC,O=%%d,N=%%d		| Uplink check revealed changed network topology, old distance (O), new distance (N)
 * | | TSF	| CHKUPL	| FAIL					| No reply received when checking uplink
 * | | TSF	| ASID		| OK,ID=%%d				| Node ID assigned
 * |!| TSF	| ASID		| FAIL,ID=%%d			| Assigned ID is invalid
 * | | TSF	| PING		| SEND,TO=%%d			| Send ping to destination (TO)
 * | | TSF	| MSG		| ACK REQ				| ACK message requested
 * | | TSF	| MSG		| ACK					| ACK message, do not proceed but forward to callback
 * | | TSF	| MSG		| FPAR RES,ID=%%d,D=%%d	| Response to find parent received from node (ID) with distance (D) to GW
 * | | TSF	| MSG		| FPAR PREF FOUND		| Preferred parent found, i.e. parent defined via MY_PARENT_NODE_ID
 * | | TSF	| MSG		| FPAR OK,ID=%%d,D=%%d	| Find parent response from node (ID) is valid, distance (D) to GW
 * | | TSF	| MSG		| FPAR INACTIVE			| Find parent response received, but no find parent request active, skip response
 * | | TSF	| MSG		| FPAR REQ,ID=%%d		| Find parent request from node (ID)
 * | | TSF	| MSG		| PINGED,ID=%%d,HP=%%d	| Node pinged by node (ID) with (HP) hops
 * | | TSF	| MSG		| PONG RECV,HP=%%d		| Pinged node replied with (HP) hops
 * | | TSF	| MSG		| BC					| Broadcast message received
 * | | TSF	| MSG		| GWL OK				| Link to GW ok
 * | | TSF	| MSG		| FWD BC MSG			| Controlled broadcast message forwarding
 * | | TSF	| MSG		| REL MSG				| Relay message
 * | | TSF	| MSG		| REL PxNG,HP=%%d		| Relay PING/PONG message, increment hop counter (HP)
 * |!| TSF	| MSG		| PVER,%%d!=%%d			| Message protocol version mismatch
 * |!| TSF	| MSG		| SIGN VERIFY FAIL		| Signing verification failed
 * |!| TSF	| MSG		| REL MSG,NORP			| Node received a message for relaying, but node is not a repeater, message skipped
 * |!| TSF	| MSG		| SIGN FAIL				| Signing message failed
 * |!| TSF	| MSG		| GWL FAIL				| GW uplink failed
 * | | TSF	| SANCHK	| OK					| Sanity check passed
 * |!| TSF	| SANCHK	| FAIL					| Sanity check failed, attempt to re-initialize radio
 * | | TSF	| CRT		| OK					| Clearing routing table successful
 * |!| TSF	| ROUTE		| FPAR ACTIVE			| Finding parent active, message not sent
 * |!| TSF	| ROUTE		| DST %%d UNKNOWN		| Routing for destination (DST) unknown, send message to parent
 * |!| TSF	| SEND		| TNR					| Transport not ready, message cannot be sent
 *
 * Incoming / outgoing messages:
 *
 * See <a href="https://www.mysensors.org/download/serial_api_20">here</a> for more detail on the format and definitons.
 * 
 * Receiving a message		
 * - TSF:MSG:READ,sender-last-destination,s=%%d,c=%%d,t=%%d,pt=%%d,l=%%d,sg=%%d:%%s		
 *
 * Sending a message		
 * - [!]TSF:MSG:SEND,sender-last-next-destination,s=%%d,c=%%d,t=%%d,pt=%%d,l=%%d,sg=%%d,ft=%%d,st=%%s:%%s
 *
 * Message fields:
 * - <b>s</b>=sensor ID
 * - <b>c</b>=command
 * - <b>t</b>=msg type
 * - <b>pt</b>=payload type
 * - <b>l</b>=length
 * - <b>ft</b>=failed uplink transmission counter 
 * - <b>sg</b>=signing flag
 *
 * @brief API declaration for MyTransport
 *
 */

#ifndef MyTransport_h
#define MyTransport_h

#include "MySensorsCore.h"

 // debug 
#if defined(MY_DEBUG)
	#define TRANSPORT_DEBUG(x,...) debug(x, ##__VA_ARGS__)	//!< debug
#else
	#define TRANSPORT_DEBUG(x,...)  //!< debug NULL
#endif

#if defined(MY_REPEATER_FEATURE)
	#define TRANSMISSION_FAILURES  10		//!< search for a new parent node after this many transmission failures, higher threshold for repeating nodes
#else
	#define TRANSMISSION_FAILURES  5		//!< search for a new parent node after this many transmission failures, lower threshold for non-repeating nodes
#endif

#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	#if defined(MY_RADIO_RFM69)
		#error Receive message buffering not supported for RFM69!
	#endif
	#if defined(MY_RS485) 
		#error Receive message buffering not supported for RS485!
	#endif
#elif !defined(MY_RX_MESSAGE_BUFFER_FEATURE) && defined(MY_RX_MESSAGE_BUFFER_SIZE)
	#error Receive message buffering requires message buffering feature enabled!
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
struct transportState {
	void(*Transition)();					//!< state transition function
	void(*Update)();						//!< state update function
};

/**
* @brief Status variables and SM state
*
* This structure stores transport status and SM variables
*/ 
typedef struct {
	transportState* currentState;			//!< pointer to current fsm state
	uint32_t stateEnter;					//!< state enter timepoint
	uint32_t lastUplinkCheck;				//!< last uplink check, required to prevent GW flooding
	uint32_t lastSanityCheck;				//!< last sanity check
	// 8 bits
	bool findingParentNode : 1;				//!< flag finding parent node is active
	bool preferredParentFound : 1;			//!< flag preferred parent found
	bool uplinkOk : 1;						//!< flag uplink ok
	bool pingActive : 1;					//!< flag ping active
	bool transportActive : 1;				//!< flag transport active
	uint8_t reserved : 3;					//!< reserved
	// 8 bits
	uint8_t retries : 4;					//!< retries / state re-enter
	uint8_t failedUplinkTransmissions : 4;	//!< counter failed uplink transmissions
	// 8 bits
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
void stReadyTransition();
/**
* @brief Monitor transport link
*/
void stReadyUpdate();
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
void transportSwitchSM(transportState& newState);
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
* @brief Call transport driver sanity check
*/
void transportInvokeSanityCheck();
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
* @return true if node ID valid and successfully assigned
*/
bool transportAssignNodeID(uint8_t newNodeId);
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
* This function is used in MyTransport and omits the transport state check, i.e. message can be sent even if transport is not ready
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
* @brief Flag transport ready
* @return true if transport is initialize and ready
*/
bool isTransportReady();
/**
* @brief Flag searching parent ongoing
* @return true if transport is searching for parent
*/
bool isTransportSearchingParent();
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
* @return true if initialization successful
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
