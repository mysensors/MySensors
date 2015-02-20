#ifndef MyParser_h
#define MyParser_h

#include "MyMessage.h"

class MyParser
{
public:
	// MyParser constructor
	MyParser();
	// parse(message, inputString)
	// parse a string into a message element
	// returns true if successfully parsed the input string
	virtual bool parse(MyMessage &message, char *inputString) = 0;
};
#endif