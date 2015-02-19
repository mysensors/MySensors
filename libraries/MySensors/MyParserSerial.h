#ifndef MyParserSerial_h
#define MyParserSerial_h

#include "MyConfig.h"
#include "MyParser.h"

class MyParserSerial : public MyParser
{ 
public:
	MyParserSerial();
	bool parse(MyMessage &message, char *inputString);
private:
	uint8_t h2i(char c);
};
#endif