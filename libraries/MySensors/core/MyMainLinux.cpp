// Initialize library and handle sketch functions like we want to

#include "MySensorCore.h"

int main(void) {
	_begin(); // Startup MySensors library

	for(;;) {
		_process();  // Process incoming data
		if (loop) loop(); // Call sketch loop
	}
	return 0;
}
