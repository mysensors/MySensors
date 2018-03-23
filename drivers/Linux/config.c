/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2018 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Based on mosquitto project, Copyright (c) 2012 Roger Light <roger@atchoo.org>
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "log.h"

static int _config_create(const char *config_file);
static int _config_parse_int(char *token, const char *name, int *value);
static int _config_parse_string(char *token, const char *name, char **value);

int config_parse(const char *config_file)
{
	FILE *fptr;
	char buf[1024];
	struct stat fileInfo;

	if (stat(config_file, &fileInfo) != 0) {
		//File does not exist.  Create it.
		logInfo("Config file %s does not exist, creating new file.\n", config_file);
		_config_create(config_file);
	}

	fptr = fopen(config_file, "rt");
	if (!fptr) {
		logError("Error opening config file \"%s\".\n", config_file);
		return -1;
	}

	conf.verbose = 4;
	conf.log_pipe = 0;
	conf.log_pipe_file = NULL;
	conf.syslog = 0;
	conf.eeprom_file = NULL;
	conf.eeprom_size = 1024;

	while (fgets(buf, 1024, fptr)) {
		if (buf[0] != '#' && buf[0] != 10 && buf[0] != 13) {
			while (buf[strlen(buf)-1] == 10 || buf[strlen(buf)-1] == 13) {
				buf[strlen(buf)-1] = 0;
			}

			if (!strncmp(buf, "verbose=", 8)) {
				char *verbose = NULL;
				if (_config_parse_string(&(buf[8]), "verbose", &verbose)) {
					fclose(fptr);
					return -1;
				} else {
					if (!strncmp(verbose, "err", 3)) {
						conf.verbose = 3;
					} else if (!strncmp(verbose, "warn", 4)) {
						conf.verbose = 4;
					} else if (!strncmp(verbose, "notice", 6)) {
						conf.verbose = 5;
					} else if (!strncmp(verbose, "info", 4)) {
						conf.verbose = 6;
					} else if (!strncmp(verbose, "debug", 5)) {
						conf.verbose = 7;
					} else {
						logError("Invalid value for verbose in configuration.\n");
						fclose(fptr);
						free(verbose);
						return -1;
					}
					free(verbose);
				}
			} else if (!strncmp(buf, "log_file=", 9)) {
				if (_config_parse_int(&(buf[9]), "log_file", &conf.log_file)) {
					fclose(fptr);
					return -1;
				} else {
					if (conf.log_file != 0 && conf.log_file != 1) {
						logError("log_file must be 1 or 0 in configuration.\n");
						fclose(fptr);
						return -1;
					}
				}
			} else if (!strncmp(buf, "log_filepath=", 13)) {
				if (_config_parse_string(&(buf[13]), "log_filepath", &conf.log_filepath)) {
					fclose(fptr);
					return -1;
				}
			} else if (!strncmp(buf, "log_pipe=", 9)) {
				if (_config_parse_int(&(buf[9]), "log_pipe", &conf.log_pipe)) {
					fclose(fptr);
					return -1;
				} else {
					if (conf.log_pipe != 0 && conf.log_pipe != 1) {
						logError("log_pipe must be 1 or 0 in configuration.\n");
						fclose(fptr);
						return -1;
					}
				}
			} else if (!strncmp(buf, "log_pipe_file=", 14)) {
				if (_config_parse_string(&(buf[14]), "log_pipe_file", &conf.log_pipe_file)) {
					fclose(fptr);
					return -1;
				}
			} else if (!strncmp(buf, "syslog=", 7)) {
				if (_config_parse_int(&(buf[7]), "syslog", &conf.syslog)) {
					fclose(fptr);
					return -1;
				} else {
					if (conf.syslog != 0 && conf.syslog != 1) {
						logError("syslog must be 1 or 0 in configuration.\n");
						fclose(fptr);
						return -1;
					}
				}
			} else if (!strncmp(buf, "eeprom_file=", 12)) {
				if (_config_parse_string(&(buf[12]), "eeprom_file", &conf.eeprom_file)) {
					fclose(fptr);
					return -1;
				}
			} else if (!strncmp(buf, "eeprom_size=", 12)) {
				if (_config_parse_int(&(buf[12]), "eeprom_size", &conf.eeprom_size)) {
					fclose(fptr);
					return -1;
				} else {
					if (conf.eeprom_size <= 0) {
						logError("eeprom_size value must be greater than 0 in configuration.\n");
						fclose(fptr);
						return -1;
					}
				}
			} else {
				logWarning("Unknown config option \"%s\".\n", buf);
			}
		}
	}
	fclose(fptr);

	if (!conf.eeprom_file) {
		logError("No eeprom_file found in configuration.\n");
		return -1;
	}

	if (conf.log_file && !conf.log_filepath) {
		logError("log_filepath must be set if you enable log_file in configuration.\n");
		return -1;
	}

	if (conf.log_pipe && !conf.log_pipe_file) {
		logError("log_pipe_file must be set if you enable log_pipe in configuration.\n");
		return -1;
	}

	return 0;
}

void config_cleanup(void)
{
	if (conf.eeprom_file) {
		free(conf.eeprom_file);
	}
	if (conf.log_pipe_file) {
		free(conf.log_pipe_file);
	}
}

int _config_create(const char *config_file)
{
	FILE *myFile;
	int ret;

	const char default_conf[] = "# Logging verbosity: debug,info,notice,warn,err\n" \
	                            "verbose=debug\n" \
	                            "# Enable logging to a file.\n" \
	                            "log_file=0\n" \
	                            "# Log file path.\n" \
	                            "log_filepath=/tmp/mysgw.log\n" \
	                            "# Enable logging to a named pipe.\n" \
	                            "# Use this option to view your gateway's log messages\n" \
	                            "# from the log_pipe_file defined bellow.\n" \
	                            "# To do so, run the following command on another terminal:\n" \
	                            "#   cat \"log_pipe_file\"\n" \
	                            "log_pipe=0\n" \
	                            "log_pipe_file=/tmp/mysgw.pipe\n" \
	                            "# Enable logging to syslog.\n" \
	                            "syslog=0\n" \
	                            "eeprom_file=/etc/mysensors.eeprom\n" \
	                            "eeprom_size=1024\n";

	myFile = fopen(config_file, "w");
	if (!myFile) {
		logError("Unable to create config file %s.\n", config_file);
		return -1;
	}
	ret = fputs(default_conf, myFile);
	fclose(myFile);

	return (ret > 0);
}

int _config_parse_int(char *token, const char *name, int *value)
{
	if (token) {
		*value = atoi(token);
	} else {
		logError("Empty %s value in configuration.\n", name);
		return 1;
	}

	return 0;
}

int _config_parse_string(char *token, const char *name, char **value)
{
	if (token) {
		if (*value) {
			logError("Duplicate %s value in configuration.\n", name);
			return 1;
		}
		while (token[0] == ' ' || token[0] == '\t') {
			token++;
		}
		*value = strdup(token);
		if (!*value) {
			logError("Out of memory.\n");
			return 1;
		}
	} else {
		logError("Empty %s value in configuration.\n", name);
		return 1;
	}
	return 0;
}
