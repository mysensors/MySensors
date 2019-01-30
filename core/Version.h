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

/**
* @file Version.h
*
* @defgroup Versiongrp Version
* @ingroup internals
* @{
*
* This file defines the MySensors library version number
* Please adjust for new releases.

* These helper macros generate a numerical and alphanumerical (see http://www.semver.org) representation of the library version number, i.e
*
* | SemVer      | Numerical   | Comments
* |-------------|-------------|------------------
* | 2.1.0       | 0x020100FF  | final
* | 2.1.1-beta  | 0x02010100  | first pre-release
* | 2.1.1       | 0x020101FF  | final
* | 2.2.0-beta  | 0x02020000  | first pre-release
* | 2.2.0-rc.1  | 0x02020001  |
* | 2.2.0-rc.2  | 0x02020002  |
* | 2.2.0       | 0x020200FF  | final
*/

#ifndef Version_h
#define Version_h

#define STR_HELPER(x) #x			//!< Helper macro, STR_HELPER()
#define STR(x) STR_HELPER(x)	//!< Helper macro, STR()

#define MYSENSORS_LIBRARY_VERSION_MAJOR							2							//!< Major release version
#define MYSENSORS_LIBRARY_VERSION_MINOR							3							//!< Minor release version
#define MYSENSORS_LIBRARY_VERSION_PATCH							2							//!< Patch version
#define MYSENSORS_LIBRARY_VERSION_PRERELEASE					"beta"						//!< Pre-release suffix, i.e. alpha, beta, rc.1, etc
#define MYSENSORS_LIBRARY_VERSION_PRERELEASE_NUMBER				0x03						//!< incremental counter, starting at 0x00. 0xFF for final release


#if (MYSENSORS_LIBRARY_VERSION_PRERELEASE_NUMBER != 0xFF)
#define MYSENSORS_LIBRARY_VERSION STR(MYSENSORS_LIBRARY_VERSION_MAJOR) "." STR(MYSENSORS_LIBRARY_VERSION_MINOR) "." STR(MYSENSORS_LIBRARY_VERSION_PATCH) "-" MYSENSORS_LIBRARY_VERSION_PRERELEASE	//!< pre-release versioning
#else
#define MYSENSORS_LIBRARY_VERSION STR(MYSENSORS_LIBRARY_VERSION_MAJOR) "." STR(MYSENSORS_LIBRARY_VERSION_MINOR) "." STR(MYSENSORS_LIBRARY_VERSION_PATCH) //!< final release versioning
#endif

#define MYSENSORS_LIBRARY_VERSION_INT ( ((uint32_t)MYSENSORS_LIBRARY_VERSION_MAJOR) << 24 | ((uint32_t)MYSENSORS_LIBRARY_VERSION_MINOR) << 16 | ((uint32_t)MYSENSORS_LIBRARY_VERSION_PATCH) << 8 | ((uint32_t)MYSENSORS_LIBRARY_VERSION_PRERELEASE_NUMBER) ) //!< numerical versioning

#endif // Version_h
/** @}*/
