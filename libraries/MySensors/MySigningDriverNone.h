/*
 Disabled signing backend. Does not provide any security measures.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#ifndef MySigningDriverNone_h
#define MySigningDriverNone_h

#include "MyConfig.h"
#include "MySigningDriver.h"
#include <stdint.h>

// The "none" signing driver that can be used for nodes
// not requiring signing
// It does check SIGNING_IDENTIFIER byte to avoid illegal mixing of signing back-ends in
// the network (as seen by this node) and it does verify proper execution order on the API.
// The "none" driver rejects all other back-ends.
class MySigningDriverNone : public MySigningDriver
{ 
public:
	MySigningDriverNone();
	bool getNonce(MyMessage &msg);
	bool checkTimer(void);
	bool putNonce(MyMessage &msg);
	bool signMsg(MyMessage &msg);
	bool verifyMsg(MyMessage &msg);
};

#endif
