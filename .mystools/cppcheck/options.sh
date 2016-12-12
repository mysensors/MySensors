#!/bin/bash

OPTIONS="--quiet                                      \
	--error-exitcode=1                                  \
	--force                                             \
	--enable=style,information                          \
	--library=${LIBRARY:-avr}                           \
	--platform="${TOOLCONFIG}"/${PLATFORM:-avr.xml}     \
 	--includes-file="${TOOLCONFIG}"/includes.cfg	      \
	--suppressions-list="${TOOLCONFIG}"/suppressions.cfg"

echo $OPTIONS
