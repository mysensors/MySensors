// Initialize library and handle sketch functions like we want to

#include <iostream>
#include <csignal>
#include <cstdlib>
#include "MySensorsCore.h"

/*
 * handler for SIGINT signal
 */
void handle_sigint(int sig)
{
	if (sig == SIGINT) {
		std::cout << "Received SIGINT\n" << std::endl;
	} else if (sig == SIGTERM) {
		std::cout << "Received SIGTERM\n" << std::endl;
	}

	#ifdef MY_RF24_IRQ_PIN
		detachInterrupt(MY_RF24_IRQ_PIN);
	#endif

	exit(0);
}

int main(void) {
	/* register the signal handler */
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);

	_begin(); // Startup MySensors library

	for (;;) {
		_process();  // Process incoming data
		if (loop) loop(); // Call sketch loop
	}
	return 0;
}
