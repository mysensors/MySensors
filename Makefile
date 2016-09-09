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

GATEWAY_BIN=mysGateway
GATEWAY=examples_linux/$(GATEWAY_BIN)
GATEWAY_C_SOURCES=$(wildcard drivers/Linux/*.c)
GATEWAY_CPP_SOURCES=$(wildcard drivers/Linux/*.cpp) examples_linux/mysGateway.cpp
OBJECTS=$(patsubst %.c,%.o,$(GATEWAY_C_SOURCES)) $(patsubst %.cpp,%.o,$(GATEWAY_CPP_SOURCES))

DEPS+=$(patsubst %.c,%.d,$(GATEWAY_C_SOURCES)) $(patsubst %.cpp,%.d,$(GATEWAY_CPP_SOURCES))

CINCLUDE=-I. -I./core -I./drivers/Linux

ifeq ($(SOC),$(filter $(SOC),BCM2835 BCM2836))
RPI_C_SOURCES=$(wildcard drivers/RPi/*.c)
RPI_CPP_SOURCES=$(wildcard drivers/RPi/*.cpp)
OBJECTS+=$(patsubst %.c,%.o,$(RPI_C_SOURCES)) $(patsubst %.cpp,%.o,$(RPI_CPP_SOURCES))

DEPS+=$(patsubst %.c,%.d,$(RPI_C_SOURCES)) $(patsubst %.cpp,%.d,$(RPI_CPP_SOURCES))

CINCLUDE+=-I./drivers/RPi
endif

.PHONY: all gateway cleanconfig clean install uninstall force

all: $(GATEWAY)

# Gateway Build
$(GATEWAY): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS)

# Include all .d files
-include $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CINCLUDE) -MMD -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(CINCLUDE) -MMD -c -o $@ $<

# clear configuration files
cleanconfig:
	@echo "[Cleaning configuration]"
	rm -rf $(CONFIG_FILE)

# clear build files
clean:
	@echo "[Cleaning]"
	rm -rf build $(OBJECTS) $(GATEWAY) $(DEPS)

$(CONFIG_FILE):
	@echo "[Running configure]"
	@./configure --no-clean

install: all install-gateway install-initscripts

install-gateway: 
	@echo "Installing $(GATEWAY) to $(GATEWAY_DIR)"
	@install -m 0755 $(GATEWAY) $(GATEWAY_DIR)

install-initscripts:
ifeq ($(INIT_SYSTEM), systemd)
	install -m0644 initscripts/mysgateway.systemd /etc/systemd/system/mysgateway.service
	@sed -i -e "s|%gateway_dir%|${GATEWAY_DIR}|g" /etc/systemd/system/mysgateway.service
	systemctl daemon-reload
	@echo "MySensors gateway has been installed, to add to the boot run:"
	@echo "  sudo systemctl enable mysgateway.service"
	@echo "To start the gateway run:"
	@echo "  sudo systemctl start mysgateway.service"
else ifeq ($(INIT_SYSTEM), sysvinit)
	install -m0755 initscripts/mysgateway.sysvinit /etc/init.d/mysgateway
	@sed -i -e "s|%gateway_dir%|${GATEWAY_DIR}|g" /etc/init.d/mysgateway
	@echo "MySensors gateway has been installed, to add to the boot run:"
	@echo "  sudo update-rc.d mysgateway defaults"
	@echo "To start the gateway run:"
	@echo "  sudo service mysgateway start"
endif

uninstall:
ifeq ($(INIT_SYSTEM), systemd)
	@echo "Stopping daemon mysgateway (ignore errors)"
	-@systemctl stop mysgateway.service
	@echo "removing files"
	rm /etc/systemd/system/mysgateway.service $(GATEWAY_DIR)/$(GATEWAY_BIN)
else ifeq ($(INIT_SYSTEM), sysvinit)
	@echo "Stopping daemon mysgateway (ignore errors)"
	-@service mysgateway stop
	@echo "removing files"
	rm /etc/init.d/mysgateway $(GATEWAY_DIR)/$(GATEWAY_BIN)
endif
