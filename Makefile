#############################################################################
#
# Makefile for MySensors
#
# Description:
# ------------
# use make all and make install to install the gateway
#

CONFIG_FILE=Makefile.inc

include $(CONFIG_FILE)

GATEWAY=examples_RPi/MySGateway
GATEWAY_SOURCES=examples_RPi/MySGateway.cpp
GATEWAY_OBJECTS=$(patsubst %.cpp,%.o,$(GATEWAY_SOURCES))
DEPS+=$(patsubst %.cpp,%.d,$(GATEWAY_SOURCES))

CINCLUDE=-I. -I./core -I./drivers/Linux -I$(RF24H_LIB_DIR)

.PHONY: all gateway cleanconfig clean install uninstall force

all: $(GATEWAY)

# Gateway Build
$(GATEWAY): $(GATEWAY_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(GATEWAY_OBJECTS)

# Include all .d files
-include $(DEPS)
	
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CINCLUDE) -MMD -c -o $@ $<

# clear configuration files
cleanconfig:
	@echo "[Cleaning configuration]"
	rm -rf $(CONFIG_FILE)

# clear build files
clean:
	@echo "[Cleaning]"
	rm -rf build $(GATEWAY_OBJECTS) $(GATEWAY) $(DEPS)

$(CONFIG_FILE):
	@echo "[Running configure]"
	@./configure --no-clean

install: all install-gateway install-initscripts

install-gateway: 
	@echo "Installing $(GATEWAY) to $(GATEWAY_DIR)"
	@install -m 0755 $(GATEWAY) $(GATEWAY_DIR)

install-initscripts:
	if [[ `systemctl` =~ -\.mount ]]; then \
		install -m0644 initscripts/mysgateway.systemd /etc/systemd/system/mysgateway.service && \
		sed -i -e "s|%gateway_dir%|${GATEWAY_DIR}|g" /etc/systemd/system/mysgateway.service && \
		systemctl daemon-reload; \
	elif [[ -f /etc/init.d/cron && ! -h /etc/init.d/cron ]]; then \
		install -m0755 initscripts/mysgateway.sysvinit /etc/init.d/mysgateway && \
		sed -i -e "s|%gateway_dir%|${GATEWAY_DIR}|g" /etc/init.d/mysgateway; \
	fi
