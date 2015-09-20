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
 *
 *******************************
 *
 * DESCRIPTION
 * Signing support created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
 * Disabled signing backend. Does not provide any security measures.
 *
 */


#ifndef MySigningNone_h
#define MySigningNone_h

#include "MyConfig.h"
#include "MySigning.h"
#include <stdint.h>

// The "none" signing driver that can be used for nodes
// not requiring signing
// It does check SIGNING_IDENTIFIER byte to avoid illegal mixing of signing back-ends in
// the network (as seen by this node) and it does verify proper execution order on the API.
// The "none" driver rejects all other back-ends.
class MySigningNone : public MySigning
{ 
public:
	MySigningNone();
	bool getNonce(MyMessage &msg);
	bool checkTimer(void);
	bool putNonce(MyMessage &msg);
	bool signMsg(MyMessage &msg);
	bool verifyMsg(MyMessage &msg);
};

#endif
