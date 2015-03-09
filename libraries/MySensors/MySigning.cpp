#include "MySigning.h"

MySigning::MySigning(bool requestSignatures) : _requestSignatures(requestSignatures) {
}


bool MySigning::requestSignatures() {
	return _requestSignatures;
}
