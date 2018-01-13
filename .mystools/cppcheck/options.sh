#!/bin/bash

OPTIONS="--quiet                                      \
	--error-exitcode=1                                  \
	--force                                             \
	--enable=style,information                          \
	-DCPPCHECK                                          \
	--language=c++                                      \
	--library=${LIBRARY:-avr}                           \
	--platform="${TOOLCONFIG}"/${PLATFORM:-avr.xml}     \
 	--includes-file="${TOOLCONFIG}"/includes.cfg	      \
 	--inline-suppr                                      \
	--suppressions-list="${TOOLCONFIG}"/suppressions.cfg"

echo $OPTIONS
