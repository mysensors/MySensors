#############################################################################
#
# Makefile for AES on Raspberry Pi
#
# License: GPL (General Public License)
# Author:  Georgios Spanos (spaniakos) <spaniakos@gmail.com>
# Date:    02/12/2014
#
# Description:
# ------------
# use make all and mak install to install the library 
# You can change the install directory by editing the LIBDIR line
#
PREFIX=/usr/local

# Library parameters
# where to put the lib
LIBDIR=$(PREFIX)/lib
# lib name 
LIB=libAES
# shared library name
LIBNAME=$(LIB).so.1.0

# Where to put the header files
HEADER_DIR=${PREFIX}/include/AES

# The recommended compiler flags for the Raspberry Pi
CCFLAGS=-Ofast -mfpu=vfp -mfloat-abi=hard -march=armv6zk -mtune=arm1176jzf-s

# make all
# reinstall the library after each recompilation
all: libAES

# Make the library
libAES: AES.o
	g++ -shared -Wl,-soname,$@.so.1 ${CCFLAGS} -o ${LIBNAME} $^

# Library parts
AES.o: AES.cpp
	g++ -Wall -fPIC ${CCFLAGS} -c $^

# clear build files
clean:
	rm -rf *.o ${LIB}.*
	rm -rf ${LIBDIR}/${LIB}.*
	rm -rf ${HEADER_DIR}

install: all install-libs install-headers

# Install the library to LIBPATH
install-libs: 
	@echo "[Installing Libs]"
	@if ( test ! -d $(PREFIX)/lib ) ; then mkdir -p $(PREFIX)/lib ; fi
	@install -m 0755 ${LIBNAME} ${LIBDIR}
	@ln -sf ${LIBDIR}/${LIBNAME} ${LIBDIR}/${LIB}.so.1
	@ln -sf ${LIBDIR}/${LIBNAME} ${LIBDIR}/${LIB}.so
	@ldconfig

install-headers:
	@echo "[Installing Headers]"
	@if ( test ! -d ${HEADER_DIR} ) ; then mkdir -p ${HEADER_DIR} ; fi
	@install -m 0644 *.h ${HEADER_DIR}
