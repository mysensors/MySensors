/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
// Initialize library and handle sketch functions like we want to

#include <stdio.h>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <getopt.h>
#include "log.h"
#include "MySensorsCore.h"

void handle_sigint(int sig)
{
	if (sig == SIGINT) {
		logNotice("Received SIGINT\n\n");
	} else if (sig == SIGTERM) {
		logNotice("Received SIGTERM\n\n");
	} else {
		return;
	}

#ifdef MY_RF24_IRQ_PIN
	detachInterrupt(MY_RF24_IRQ_PIN);
#endif

#if defined(MY_GATEWAY_SERIAL)
	MY_SERIALDEVICE.end();
#endif

	closelog();

	exit(EXIT_SUCCESS);
}

static int daemonize(void)
{
	pid_t pid, sid;

	/* Fork off the parent process */
	pid = fork();
	if (pid < 0) {
		logError("fork: %s\n", strerror(errno));
		return -1;
	}
	/* If we got a good PID, then we can exit the parent process. */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* At this point we are executing as the child process */

	/* Change the file mode mask */
	umask(0);

	/* Create a new SID for the child process */
	sid = setsid();
	if (sid < 0) {
		logError("setsid: %s\n", strerror(errno));
		return -1;
	}

	/* Change the current working directory.  This prevents the current
	directory from being locked; hence not being able to remove it. */
	if ((chdir("/")) < 0) {
		logError("chdir(\"/\"): %s\n", strerror(errno));
		return -1;
	}

	if (freopen( "/dev/null", "r", stdin) == NULL) {
		logError("freopen: %s\n", strerror(errno));
	}
	if (freopen( "/dev/null", "r", stdout) == NULL) {
		logError("freopen: %s\n", strerror(errno));
	}
	if (freopen( "/dev/null", "r", stderr) == NULL) {
		logError("freopen: %s\n", strerror(errno));
	}

	return 0;
}

void print_usage()
{
	printf("Usage: mysgw [options]\n\n" \
	       "Options:\n" \
	       "  -h, --help                 Display a short summary of all program options.\n" \
	       "  -d, --debug                Enable debug.\n" \
	       "  -b, --background           Run as a background process.\n"
	       "  --gen-soft-hmac-key        Generate and print a soft hmac key.\n"
	       "  --gen-soft-serial-key      Generate and print a soft serial key.\n"
	       "  --gen-aes-key              Generate and print an aes encryption key.\n"
	       "  --print-soft-hmac-key      Print the soft hmac key from the config file.\n"
	       "  --print-soft-serial-key    Print the soft serial key from the config file.\n"
	       "  --print-aes-key            Print the aes encryption key from the config file.\n"
	       "  --set-soft-hmac-key        Write a soft hmac key to the config file.\n"
	       "  --set-soft-serial-key      Write a soft serial key to the config file.\n"
	       "  --set-aes-key              Write an aes encryption key to the config file.\n");
}

void print_soft_sign_hmac_key(uint8_t *key_ptr = NULL)
{
	uint8_t key[32];

	if (key_ptr == NULL) {
		hwReadConfigBlock(&key, reinterpret_cast<void*>EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS, 32);
		key_ptr = key;
	}

	printf("SOFT_HMAC_KEY | ");
	for (int i = 0; i < 32; i++) {
		printf("%02X", key_ptr[i]);
	}
	printf("\n\n");

	printf("The next line is intended to be used in SecurityPersonalizer.ino:\n");
	printf("#define MY_SOFT_HMAC_KEY ");
	for (int i=0; i<32; i++) {
		printf("%#02X", key_ptr[i]);
		if (i < 31) {
			printf(",");
		}
	}
	printf("\n\n");
}

void generate_soft_sign_hmac_key()
{
	uint8_t key[32];

	for (int i = 0; i < 32; i++) {
		key[i] = random(256) ^ micros();
		unsigned long enter = hwMillis();
		while (hwMillis() - enter < (unsigned long)2);
	}

	print_soft_sign_hmac_key(key);

	printf("To use this key, run mysgw with:\n"
	       " --set-soft-hmac-key=");
	for (int i = 0; i < 32; i++) {
		printf("%02X", key[i]);
	}
	printf("\n");
}

void set_soft_sign_hmac_key(char *key_str)
{
	uint8_t key[32];

	if (strlen(key_str) != 64) {
		printf("invalid key!\n");
	} else {
		for (int i = 0; i < 64; ++i) {
			int n;
			char c = key_str[i];
			if (c <= '9') {
				n = c - '0';
			} else if (c >= 'a') {
				n = c - 'a' + 10;
			} else {
				n = c - 'A' + 10;
			}

			if ((i & 0x1) == 0) {
				key[i/2] = n * 16;
			} else {
				key[i/2] += n;
			}
		}
		hwWriteConfigBlock(&key, reinterpret_cast<void*>EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS, 32);
		print_soft_sign_hmac_key();
	}
}

void print_soft_sign_serial_key(uint8_t *key_ptr = NULL)
{
	uint8_t key[9];

	if (key_ptr == NULL) {
		hwReadConfigBlock(&key, reinterpret_cast<void*>EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, 9);
		key_ptr = key;
	}

	printf("SOFT_SERIAL   | ");
	for (int i = 0; i < 9; i++) {
		printf("%02X", key_ptr[i]);
	}
	printf("\n\n");

	printf("The next line is intended to be used in SecurityPersonalizer.ino:\n");
	printf("#define MY_SOFT_SERIAL ");
	for (int i=0; i<9; i++) {
		printf("%#02X", key_ptr[i]);
		if (i < 8) {
			printf(",");
		}
	}
	printf("\n\n");
}

void generate_soft_sign_serial_key()
{
	uint8_t key[9];

	for (int i = 0; i < 9; i++) {
		key[i] = random(256) ^ micros();
		unsigned long enter = hwMillis();
		while (hwMillis() - enter < (unsigned long)2);
	}

	print_soft_sign_serial_key(key);

	printf("To use this key, run mysgw with:\n"
	       " --set-soft-serial-key=");
	for (int i = 0; i < 9; i++) {
		printf("%02X", key[i]);
	}
	printf("\n");
}

void set_soft_sign_serial_key(char *key_str)
{
	uint8_t key[9];

	if (strlen(key_str) != 18) {
		printf("invalid key!\n");
	} else {
		for (int i = 0; i < 18; ++i) {
			int n;
			char c = key_str[i];
			if (c <= '9') {
				n = c - '0';
			} else if (c >= 'a') {
				n = c - 'a' + 10;
			} else {
				n = c - 'A' + 10;
			}

			if ((i & 0x1) == 0) {
				key[i/2] = n * 16;
			} else {
				key[i/2] += n;
			}
		}
		hwWriteConfigBlock(&key, reinterpret_cast<void*>EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, 9);
		print_soft_sign_serial_key();
	}
}

void print_aes_key(uint8_t *key_ptr = NULL)
{
	uint8_t key[16];

	if (key_ptr == NULL) {
		hwReadConfigBlock(&key, reinterpret_cast<void*>EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
		key_ptr = key;
	}

	printf("AES_KEY       | ");
	for (int i = 0; i < 16; i++) {
		printf("%02X", key_ptr[i]);
	}
	printf("\n\n");

	printf("The next line is intended to be used in SecurityPersonalizer.ino:\n");
	printf("#define MY_AES_KEY ");
	for (int i=0; i<16; i++) {
		printf("%#02X", key_ptr[i]);
		if (i < 15) {
			printf(",");
		}
	}
	printf("\n\n");
}

void generate_aes_key()
{
	uint8_t key[16];

	for (int i = 0; i < 16; i++) {
		key[i] = random(256) ^ micros();
		unsigned long enter = hwMillis();
		while (hwMillis() - enter < (unsigned long)2);
	}

	print_aes_key(key);

	printf("To use this key, run mysgw with:\n"
	       " --set-aes-key=");
	for (int i = 0; i < 16; i++) {
		printf("%02X", key[i]);
	}
	printf("\n");
}

void set_aes_key(char *key_str)
{
	uint8_t key[16];

	if (strlen(key_str) != 32) {
		printf("invalid key!\n");
	} else {
		for (int i = 0; i < 32; ++i) {
			int n;
			char c = key_str[i];
			if (c <= '9') {
				n = c - '0';
			} else if (c >= 'a') {
				n = c - 'a' + 10;
			} else {
				n = c - 'A' + 10;
			}

			if ((i & 0x1) == 0) {
				key[i/2] = n * 16;
			} else {
				key[i/2] += n;
			}
		}
		hwWriteConfigBlock(&key, reinterpret_cast<void*>EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
		print_aes_key();
	}
}

int main(int argc, char *argv[])
{
	int opt, log_opts, debug = 0, foreground = 1;
	char *key = NULL;

	/* register the signal handler */
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);

	hwRandomNumberInit();

	static struct option long_options[] = {
		{"help",					no_argument,		0,	'h'},
		{"debug",					no_argument,		0,	'd'},
		{"background",				no_argument,		0,	'b'},
		{"gen-soft-hmac-key",		no_argument,		0,	'A'},
		{"gen-soft-serial-key",		no_argument,		0,	'B'},
		{"gen-aes-key",				no_argument,		0,	'C'},
		{"print-soft-hmac-key",		no_argument,		0,	'D'},
		{"print-soft-serial-key",	no_argument,		0,	'E'},
		{"print-aes-key",			no_argument,		0,	'F'},
		{"set-soft-hmac-key",		required_argument,	0,	'G'},
		{"set-soft-serial-key",		required_argument,	0,	'H'},
		{"set-aes-key",				required_argument,	0,	'I'},
		{0, 0, 0, 0}
	};

	int long_index = 0;
	while ((opt = getopt_long(argc, argv,"hdbABCDEFGHI", long_options, &long_index )) != -1) {
		switch (opt) {
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'd':
			debug = 1;
			break;
		case 'b':
			foreground = 0;
			break;
		case 'A':
			generate_soft_sign_hmac_key();
			exit(EXIT_SUCCESS);
		case 'B':
			generate_soft_sign_serial_key();
			exit(EXIT_SUCCESS);
		case 'C':
			generate_aes_key();
			exit(EXIT_SUCCESS);
		case 'D':
			print_soft_sign_hmac_key();
			exit(EXIT_SUCCESS);
		case 'E':
			print_soft_sign_serial_key();
			exit(EXIT_SUCCESS);
		case 'F':
			print_aes_key();
			exit(EXIT_SUCCESS);
		case 'G':
			key = strdup(optarg);
			set_soft_sign_hmac_key(key);
			exit(EXIT_SUCCESS);
		case 'H':
			key = strdup(optarg);
			set_soft_sign_serial_key(key);
			exit(EXIT_SUCCESS);
		case 'I':
			key = strdup(optarg);
			set_aes_key(key);
			exit(EXIT_SUCCESS);
		default:
			print_usage();
			exit(EXIT_SUCCESS);
		}
	}

	log_opts = LOG_CONS;
	if (foreground && isatty(STDIN_FILENO)) {
		// Also print syslog to stderror
		log_opts |= LOG_PERROR;
	}
	if (!debug) {
		// Ignore debug type messages
		setlogmask(LOG_UPTO (LOG_INFO));
	}
	logOpen(log_opts, LOG_USER);

	if (!foreground && !debug) {
		if (daemonize() != 0) {
			exit(EXIT_FAILURE);
		}
	}

	logInfo("Starting gateway...\n");
	logInfo("Protocol version - %s\n", MYSENSORS_LIBRARY_VERSION);

	_begin(); // Startup MySensors library

	for (;;) {
		_process();  // Process incoming data
		if (loop) {
			loop(); // Call sketch loop
		}
	}
	return 0;
}
