##########################################################################
# Configurable options                                                   #
##########################################################################
# Install Base location
PREFIX=/usr/local
# Bin Dir
BINDIR=$(PREFIX)/sbin

##########################################################################
# Please do not change anything below this line                          #
##########################################################################
CXX ?= g++

ifeq "$(shell uname -m)" "armv6l"
	ARCH=armv6zk
endif
ifeq "$(shell uname -m)" "armv7l"
	ARCH=armv7-a
endif
ifdef ARCH
	# The recommended compiler flags for the Raspberry Pi
	CXXFLAGS+=-Ofast -mfpu=vfp -mfloat-abi=hard -march=$(ARCH) -mtune=arm1176jzf-s -DRASPBERRYPI_ARCH $(OPTFLAGS)
	LDFLAGS+=-lrf24-bcm
endif

CXXFLAGS+=-g -Wall -Wextra
CXXFLAGS+=-DLINUX_ARCH_GENERIC $(OPTFLAGS)

# get PI Revision from cpuinfo
PIREV := $(shell cat /proc/cpuinfo | grep Revision | cut -f 2 -d ":" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$$//')
ifeq ($(PIREV),$(filter $(PIREV),a01041 a21041 0010))
	# a01041 and a21041 are PI 2 Model B with BPLUS Layout and 0010 is Pi Model B+ with BPLUS Layout
	CXXFLAGS+=-D__PI_BPLUS
endif 

GATEWAY=examples_RPi/PiGateway
GATEWAY_SOURCES=examples_RPi/PiGateway.cpp $(wildcard ./utility/*.cpp)
GATEWAY_OBJECTS=$(patsubst %.cpp,%.o,$(GATEWAY_SOURCES))
DEPS+=$(patsubst %.cpp,%.d,$(GATEWAY_SOURCES))

RF24H = /usr/local/include/RF24
CINCLUDE=-I. -I./core -I$(RF24H)

.PHONY: all clean install uninstall

all: $(GATEWAY)

# Basic Gateway Build
$(GATEWAY): LDFLAGS += -pthread
$(GATEWAY): $(GATEWAY_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(GATEWAY_OBJECTS)

# Include all .d files
-include $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CINCLUDE) -MMD -c -o $@ $<

# The Cleaner
clean:
	rm -rf build $(GATEWAY_OBJECTS) $(GATEWAY) $(DEPS)

install: all install-gatewaybasic install-gatewayserial install-gatewayethernet install-initscripts

install-gatewaybasic: 
	@echo "Installing $(GATEWAY) to $(BINDIR)"
	@install -m 0755 $(GATEWAY) $(BINDIR)
