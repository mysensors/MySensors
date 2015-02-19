/*
 Dummy signing backend. Does not provide any security measures.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#ifndef MySigningDriverDummy_h
#define MySigningDriverDummy_h

#include "MyConfig.h"
#include "MySigningDriver.h"
#include <stdint.h>

// The dummy signing driver that can be used for nodes
// not requiring signing
// It does check SIGNING_IDENTIFIER byte to avoid illegal mixing of signing back-ends in
// the network (as seen by this node) and it does verify proper execution order on the API.
// The dummy driver rejects all other back-ends.
class MySigningDriverDummy : public MySigningDriver
{ 
public:
	MySigningDriverDummy();
	bool getNonce(MyMessage &msg);
	bool checkTimer(void);
	bool putNonce(MyMessage &msg);
	bool signMsg(MyMessage &msg);
	bool verifyMsg(MyMessage &msg);
};

#endif
