/*
 Disabled signing backend. Does not provide any security measures.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#ifndef MySigningNone_h
#define MySigningNone_h

#include "MyConfig.h"
#include "MySigning.h"
#include <stdint.h>

#define SIGNING_IDENTIFIER (0)

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
