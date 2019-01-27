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

#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

struct config {
	int verbose;
	int log_file;
	char *log_filepath;
	int log_pipe;
	char *log_pipe_file;
	int syslog;
	char *eeprom_file;
	int eeprom_size;
	char *soft_hmac_key;
	char *soft_serial_key;
	char *aes_key;
} conf;

int config_parse(const char *config_file);
void config_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif