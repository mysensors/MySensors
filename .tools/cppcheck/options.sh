#!/bin/bash

OPTIONS="--quiet                                      \
	--error-exitcode=1                                  \
	--force                                             \
	--enable=style,information                          \
	--library=avr                                       \
	--platform="${TOOLCONFIG}"/avr.xml                  \
 	--includes-file="${TOOLCONFIG}"/includes.cfg	      \
	--suppressions-list="${TOOLCONFIG}"/suppressions.cfg"

echo $OPTIONS
