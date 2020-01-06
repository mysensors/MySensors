/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
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
#include "config.h"
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

	logClose();

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
	       "  -c, --config-file          Config file. [" MY_LINUX_CONFIG_FILE "]\n" \
	       "  -h, --help                 Display a short summary of all program options.\n" \
	       "  -q, --quiet                Quiet mode, disable log messages written to the terminal.\n" \
	       "  --daemon                   Run as a daemon.\n" \
	       "  --gen-soft-hmac-key        Generate and print a soft hmac key.\n" \
	       "  --gen-soft-serial-key      Generate and print a soft serial key.\n" \
	       "  --gen-aes-key              Generate and print an aes encryption key.\n");
}

void print_soft_sign_hmac_key(uint8_t *key_ptr = NULL)
{
	uint8_t key[32];

	if (key_ptr == NULL) {
		hwReadConfigBlock(&key, reinterpret_cast<void*>EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS, 32);
		key_ptr = key;
	}

	printf("soft_hmac_key=");
	for (int i = 0; i < 32; i++) {
		printf("%02X", key_ptr[i]);
	}
	printf("\n\n");

	printf("The next line is intended to be used in SecurityPersonalizer.ino:\n");
	printf("#define MY_HMAC_KEY ");
	for (int i=0; i<32; i++) {
		printf("%#02X", key_ptr[i]);
		if (i < 31) {
			printf(",");
		}
	}
	printf("\n\n");
}

void generate_soft_sign_hmac_key(char *config_file = NULL)
{
	uint8_t key[32];

	printf("Generating key...");
	while (hwGetentropy(&key, sizeof(key)) != sizeof(key));
	printf(" done.\n");

	printf("To use the new key, update the value in %s witn:\n",
	       config_file?config_file:MY_LINUX_CONFIG_FILE);
	print_soft_sign_hmac_key(key);

#if defined(MY_SIGNING_SIMPLE_PASSWD)
	printf("Note: The gateway was built with simplified signing using the password: %s\n" \
	       "      Any key set with soft_hmac_key option in the config file is ignored.\n\n",
	       MY_SIGNING_SIMPLE_PASSWD);
#elif !defined(MY_SIGNING_FEATURE)
	printf("Note: The gateway was not built with signing support.\n" \
	       "      Any key set with soft_hmac_key option in the config file is ignored.\n\n");
#endif
}

void set_soft_sign_hmac_key(char *key_str)
{
	uint8_t key[32];

	if (strlen(key_str) != 64) {
		logWarning("Invalid HMAC key!\n");
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
	}
}

void print_soft_sign_serial_key(uint8_t *key_ptr = NULL)
{
	uint8_t key[9];

	if (key_ptr == NULL) {
		hwReadConfigBlock(&key, reinterpret_cast<void*>EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, 9);
		key_ptr = key;
	}

	printf("soft_serial_key=");
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

void generate_soft_sign_serial_key(char *config_file = NULL)
{
	uint8_t key[9];

	printf("Generating key...");
	while (hwGetentropy(&key, sizeof(key)) != sizeof(key));
	printf(" done.\n");

	printf("To use the new key, update the value in %s witn:\n",
	       config_file?config_file:MY_LINUX_CONFIG_FILE);
	print_soft_sign_serial_key(key);

#if defined(MY_SIGNING_SIMPLE_PASSWD)
	printf("Note: The gateway was built with simplified signing using the password: %s\n" \
	       "      Any key set with soft_serial_key option in the config file is ignored.\n\n",
	       MY_SIGNING_SIMPLE_PASSWD);
#elif !defined(MY_SIGNING_FEATURE)
	printf("Note: The gateway was not built with signing support.\n" \
	       "      Any key set with soft_serial_key option in the config file is ignored.\n\n");
#endif
}

void set_soft_sign_serial_key(char *key_str)
{
	uint8_t key[9];

	if (strlen(key_str) != 18) {
		logWarning("Invalid soft serial key!\n");
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
	}
}

void print_aes_key(uint8_t *key_ptr = NULL)
{
	uint8_t key[16];

	if (key_ptr == NULL) {
		hwReadConfigBlock(&key, reinterpret_cast<void*>EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
		key_ptr = key;
	}

	printf("aes_key=");
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

void generate_aes_key(char *config_file = NULL)
{
	uint8_t key[16];

	printf("Generating key...");
	while (hwGetentropy(&key, sizeof(key)) != sizeof(key));
	printf(" done.\n");

	printf("To use the new key, update the value in %s witn:\n",
	       config_file?config_file:MY_LINUX_CONFIG_FILE);
	print_aes_key(key);

#if defined(MY_ENCRYPTION_SIMPLE_PASSWD)
	printf("Note: The gateway was built with simplified encryption using the password: %s\n" \
	       "      Any key set with aes_key option in the config file is ignored.\n\n",
	       MY_ENCRYPTION_SIMPLE_PASSWD);
#elif !defined(MY_ENCRYPTION_FEATURE)
	printf("Note: The gateway was not built with encryption support.\n" \
	       "      Any key set with aes_key option in the config file is ignored.\n\n");
#endif
}

void set_aes_key(char *key_str)
{
	uint8_t key[16];

	if (strlen(key_str) != 32) {
		logWarning("Invalid AES key!\n");
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
	}
}

int main(int argc, char *argv[])
{
	int opt, daemon = 0, quiet = 0;
	char *config_file = NULL;
	bool gen_soft_sign_hmac_key = false;
	bool gen_soft_sign_serial_key = false;
	bool gen_aes_key = false;

	/* register the signal handler */
	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigint);
	signal(SIGPIPE, handle_sigint);

	hwRandomNumberInit();

	static struct option long_options[] = {
		{"config-file",				required_argument,	0,	'c'},
		{"daemon",					no_argument,		0,	'J'},
		{"help",					no_argument,		0,	'h'},
		{"quiet",					no_argument,		0,	'q'},
		{"gen-soft-hmac-key",		no_argument,		0,	'A'},
		{"gen-soft-serial-key",		no_argument,		0,	'B'},
		{"gen-aes-key",				no_argument,		0,	'C'},
		{0, 0, 0, 0}
	};

	int long_index = 0;
	while ((opt = getopt_long(argc, argv,"chqABCJ", long_options, &long_index )) != -1) {
		switch (opt) {
		case 'c':
			config_file = strdup(optarg);
			break;
		case 'h':
			print_usage();
			exit(EXIT_SUCCESS);
		case 'q':
			quiet = 1;
			break;
		case 'A':
			gen_soft_sign_hmac_key = true;
			break;
		case 'B':
			gen_soft_sign_serial_key = true;
			break;
		case 'C':
			gen_aes_key = true;
			break;
		case 'J':
			daemon = 1;
			break;
		default:
			print_usage();
			exit(EXIT_SUCCESS);
		}
	}

	if (gen_soft_sign_hmac_key || gen_soft_sign_serial_key || gen_aes_key) {
		if (gen_soft_sign_hmac_key) {
			generate_soft_sign_hmac_key(config_file);
		}
		if (gen_soft_sign_serial_key) {
			generate_soft_sign_serial_key(config_file);
		}
		if (gen_aes_key) {
			generate_aes_key(config_file);
		}
		exit(EXIT_SUCCESS);
	}

	if (daemon) {
		if (daemonize() != 0) {
			exit(EXIT_FAILURE);
		}
		quiet = 1;
	}

	if (config_parse(config_file?config_file:MY_LINUX_CONFIG_FILE) != 0) {
		exit(EXIT_FAILURE);
	}

	logSetQuiet(quiet);
	logSetLevel(conf.verbose);

	if (conf.log_file) {
		if (logSetFile(conf.log_filepath) != 0) {
			logError("Failed to open log file.\n");
		}
	}

	if (conf.log_pipe) {
		if (logSetPipe(conf.log_pipe_file) != 0) {
			logError("Failed to open log pipe.\n");
		}
	}

	if (conf.syslog) {
		logSetSyslog(LOG_CONS, LOG_USER);
	}

	logInfo("Starting gateway...\n");
	logInfo("Protocol version - %s\n", MYSENSORS_LIBRARY_VERSION);

	_begin(); // Startup MySensors library

	// EEPROM is initialized within _begin()
	// any operation on it must be done hereafter

#if defined(MY_SIGNING_FEATURE) && !defined(MY_SIGNING_SIMPLE_PASSWD)
	// Check if we need to update the signing keys in EEPROM
	if (conf.soft_hmac_key) {
		set_soft_sign_hmac_key(conf.soft_hmac_key);
	} else {
		logError("soft_hmac_key was not found in %s\n", config_file?config_file:MY_LINUX_CONFIG_FILE);
		exit(EXIT_FAILURE);
	}
	if (conf.soft_serial_key) {
		set_soft_sign_serial_key(conf.soft_serial_key);
	} else {
		logError("soft_serial_key was not found in %s\n", config_file?config_file:MY_LINUX_CONFIG_FILE);
		exit(EXIT_FAILURE);
	}
#endif
#if defined(MY_ENCRYPTION_FEATURE) && !defined(MY_ENCRYPTION_SIMPLE_PASSWD)
	// Check if we need to update the encryption key in EEPROM
	if (conf.aes_key) {
		set_aes_key(conf.aes_key);
	} else {
		logError("aes_key was not found in %s\n", config_file?config_file:MY_LINUX_CONFIG_FILE);
		exit(EXIT_FAILURE);
	}
#endif

	if (config_file) {
		free(config_file);
	}

	for (;;) {
		_process();  // Process incoming data
		if (loop) {
			loop(); // Call sketch loop
		}
	}
	return 0;
}
