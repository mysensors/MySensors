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

COMPILER_FLAGS=examples_RPi/compiler_flags
GATEWAY=examples_RPi/PiGateway
GATEWAYMQTT=examples_RPi/PiGatewayMQTT
GATEWAY_SOURCES=examples_RPi/PiGateway.cpp $(wildcard ./utility/*.cpp)
GATEWAY_OBJECTS=$(patsubst %.cpp,%.o,$(GATEWAY_SOURCES))
DEPS+=$(patsubst %.cpp,%.d,$(GATEWAY_SOURCES))

RF24H = /usr/local/include/RF24
CINCLUDE=-I. -I./core -I$(RF24H)

.PHONY: all gateway gatewaymqtt clean install uninstall force

all: $(GATEWAY)

mqtt: $(GATEWAYMQTT)

$(COMPILER_FLAGS): force
	echo '$(CXXFLAGS)' | cmp -s - $@ || echo '$(CXXFLAGS)' > $@

$(GATEWAY_OBJECTS): $(COMPILER_FLAGS)

# Basic Gateway Build
$(GATEWAY): LDFLAGS += -pthread
$(GATEWAY): $(GATEWAY_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(GATEWAY_OBJECTS)
	
# MQTT Gateway Build
$(GATEWAYMQTT): CXXFLAGS += -DMY_GATEWAY_MQTT_CLIENT
$(GATEWAYMQTT): LDFLAGS += -pthread -lmosquitto
$(GATEWAYMQTT): $(GATEWAY_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(GATEWAY_OBJECTS)

# Include all .d files
-include $(DEPS)
	
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CINCLUDE) -MMD -c -o $@ $<

# The Cleaner
clean:
	rm -rf build $(GATEWAY_OBJECTS) $(GATEWAY) $(GATEWAYMQTT) $(DEPS) $(COMPILER_FLAGS)

install: all install-gatewaybasic install-gatewayserial install-gatewayethernet install-initscripts

install-gatewaybasic: 
	@echo "Installing $(GATEWAY) to $(BINDIR)"
	@install -m 0755 $(GATEWAY) $(BINDIR)
