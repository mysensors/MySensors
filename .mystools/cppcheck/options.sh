#!/bin/bash

OPTIONS="--quiet                                          \
	--error-exitcode=1                                    \
	--force                                               \
     --enable=style,portability,performance               \
	-DCPPCHECK                                            \
	--language=c++                                        \
	--library=${LIBRARY:-avr}                             \
	--platform="${TOOLCONFIG}"/${PLATFORM:-avr.xml}       \
 	--inline-suppr                                        \
	--suppressions-list="${TOOLCONFIG}"/suppressions.cfg"

echo $OPTIONS
