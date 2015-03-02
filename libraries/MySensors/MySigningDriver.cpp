#include "MySigningDriver.h"

MySigningDriver::MySigningDriver(bool requestSignatures) : _requestSignatures(requestSignatures) {
}


bool MySigningDriver::requestSignatures() {
	return _requestSignatures;
}
