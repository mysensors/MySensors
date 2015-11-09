// Initialize library and handle sketch functions like we want to
int main(void) {
	init();
	#if defined(USBCON)
		USBDevice.attach();
	#endif
	_begin(); // Startup MySensors library
	if (setup) setup(); // Call sketch setup

	for(;;) {
		_process();  // Process incoming data
		if (loop) loop(); // Call sketch loop
		if (serialEventRun) serialEventRun();
	}
	return 0;
}
