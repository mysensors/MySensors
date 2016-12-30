/*
Copyright (c) 2012-2016 Ben Croston

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "cpuinfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int get_rpi_info(rpi_info *info)
{
	FILE *fp;
	char buffer[1024];
	char hardware[1024];
	char revision[1024];
	char *rev;
	int found = 0;
	int len;

	if ((fp = fopen("/proc/cpuinfo", "r")) == NULL) {
		return -1;
	}
	while(!feof(fp)) {
		fgets(buffer, sizeof(buffer), fp);
		sscanf(buffer, "Hardware	: %s", hardware);
		if (strcmp(hardware, "BCM2708") == 0 ||
		        strcmp(hardware, "BCM2709") == 0 ||
		        strcmp(hardware, "BCM2835") == 0 ||
		        strcmp(hardware, "BCM2836") == 0 ||
		        strcmp(hardware, "BCM2837") == 0 ) {
			found = 1;
		}
		sscanf(buffer, "Revision	: %s", revision);
	}
	fclose(fp);

	if (!found) {
		return -1;
	}

	if ((len = strlen(revision)) == 0) {
		return -1;
	}

	if (len >= 6 && strtol((char[]) {
	revision[len-6],0
	}, NULL, 16) & 8) {
		// new scheme
		//info->rev = revision[len-1]-'0';
		strcpy(info->revision, revision);
		switch (revision[len-2]) {
		case '0':
			info->type = "Model A";
			info->p1_revision = 2;
			break;
		case '1':
			info->type = "Model B";
			info->p1_revision = 2;
			break;
		case '2':
			info->type = "Model A+";
			info->p1_revision = 3;
			break;
		case '3':
			info->type = "Model B+";
			info->p1_revision = 3;
			break;
		case '4':
			info->type = "Pi 2 Model B";
			info->p1_revision = 3;
			break;
		case '5':
			info->type = "Alpha";
			info->p1_revision = 3;
			break;
		case '6':
			info->type = "Compute";
			info->p1_revision = 0;
			break;
		case '8':
			info->type = "Pi 3 Model B";
			info->p1_revision = 3;
			break;
		case '9':
			info->type = "Zero";
			info->p1_revision = 3;
			break;
		default :
			info->type = "Unknown";
			info->p1_revision = 3;
			break;
		}
		switch (revision[len-4]) {
		case '0':
			info->processor = "BCM2835";
			break;
		case '1':
			info->processor = "BCM2836";
			break;
		case '2':
			info->processor = "BCM2837";
			break;
		default :
			info->processor = "Unknown";
			break;
		}
		switch (revision[len-5]) {
		case '0':
			info->manufacturer = "Sony";
			break;
		case '1':
			info->manufacturer = "Egoman";
			break;
		case '2':
			info->manufacturer = "Embest";
			break;
		case '4':
			info->manufacturer = "Embest";
			break;
		default :
			info->manufacturer = "Unknown";
			break;
		}
		switch (strtol((char[]) {
		revision[len-6],0
		}, NULL, 16) & 7) {
		case 0:
			info->ram = "256M";
			break;
		case 1:
			info->ram = "512M";
			break;
		case 2:
			info->ram = "1024M";
			break;
		default:
			info->ram = "Unknown";
			break;
		}
	}
	else {
		// old scheme
		info->ram = "Unknown";
		info->manufacturer = "Unknown";
		info->processor = "Unknown";
		info->type = "Unknown";
		strcpy(info->revision, revision);

		// get last four characters (ignore preceeding 1000 for overvolt)
		if (len > 4) {
			rev = (char *)&revision+len-4;
		} else {
			rev = revision;
		}

		if ((strcmp(rev, "0002") == 0) ||
		        (strcmp(rev, "0003") == 0)) {
			info->type = "Model B";
			info->p1_revision = 1;
			info->ram = "256M";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "0004") == 0) {
			info->type = "Model B";
			info->p1_revision = 2;
			info->ram = "256M";
			info->manufacturer = "Sony";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "0005") == 0) {
			info->type = "Model B";
			info->p1_revision = 2;
			info->ram = "256M";
			info->manufacturer = "Qisda";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "0006") == 0) {
			info->type = "Model B";
			info->p1_revision = 2;
			info->ram = "256M";
			info->manufacturer = "Egoman";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "0007") == 0) {
			info->type = "Model A";
			info->p1_revision = 2;
			info->ram = "256M";
			info->manufacturer = "Egoman";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "0008") == 0) {
			info->type = "Model A";
			info->p1_revision = 2;
			info->ram = "256M";
			info->manufacturer = "Sony";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "0009") == 0) {
			info->type = "Model A";
			info->p1_revision = 2;
			info->ram = "256M";
			info->manufacturer = "Qisda";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "000d") == 0) {
			info->type = "Model B";
			info->p1_revision = 2;
			info->ram = "512M";
			info->manufacturer = "Egoman";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "000e") == 0) {
			info->type = "Model B";
			info->p1_revision = 2;
			info->ram = "512M";
			info->manufacturer = "Sony";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "000f") == 0) {
			info->type = "Model B";
			info->p1_revision = 2;
			info->ram = "512M";
			info->manufacturer = "Qisda";
			info->processor = "BCM2835";
		} else if ((strcmp(rev, "0011") == 0) ||
		           (strcmp(rev, "0014") == 0)) {
			info->type = "Compute Module";
			info->p1_revision = 0;
			info->ram = "512M";
			info->processor = "BCM2835";
		} else if (strcmp(rev, "0012") == 0) {
			info->type = "Model A+";
			info->p1_revision = 3;
			info->ram = "256M";
			info->processor = "BCM2835";
		} else if ((strcmp(rev, "0010") == 0) ||
		           (strcmp(rev, "0013") == 0)) {
			info->type = "Model B+";
			info->p1_revision = 3;
			info->ram = "512M";
			info->processor = "BCM2835";
		} else {  // don't know - assume revision 3 p1 connector
			info->p1_revision = 3;
		}
	}
	return 0;
}

/*

32 bits
NEW                   23: will be 1 for the new scheme, 0 for the old scheme
MEMSIZE             20: 0=256M 1=512M 2=1G
MANUFACTURER  16: 0=SONY 1=EGOMAN
PROCESSOR         12: 0=2835 1=2836
TYPE                   04: 0=MODELA 1=MODELB 2=MODELA+ 3=MODELB+ 4=Pi2 MODEL B 5=ALPHA 6=CM
REV                     00: 0=REV0 1=REV1 2=REV2

pi2 = 1<<23 | 2<<20 | 1<<12 | 4<<4 = 0xa01040

--------------------

SRRR MMMM PPPP TTTT TTTT VVVV

S scheme (0=old, 1=new)
R RAM (0=256, 1=512, 2=1024)
M manufacturer (0='SONY',1='EGOMAN',2='EMBEST',3='UNKNOWN',4='EMBEST')
P processor (0=2835, 1=2836 2=2837)
T type (0='A', 1='B', 2='A+', 3='B+', 4='Pi 2 B', 5='Alpha', 6='Compute Module')
V revision (0-15)

*/
