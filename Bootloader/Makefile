PROJECT = MyOtaBootloader
INCLUDES = ../libraries/MySensors

MCU = atmega328p
CLK = 16000000L

ISP_PORT = com5
ISP_SPEED = 115200
ISP_PROTOCOL = stk500v2
ISP_MCU = m328p
# HFUSE = D8 - 4096 byte boot
ISP_HFUSE = D8
# LFUSE = 4E - Low power resonator (16MHz), fast start, clkdiv/8
ISP_LFUSE = FF
# EFUSE = 06 - 1.8V brownout
ISP_EFUSE = 06
ISP_ARGS = -c$(ISP_PROTOCOL) -P$(ISP_PORT) -b$(ISP_SPEED) -p$(ISP_MCU)

ifeq ($(OS),Windows_NT)
	BINPATH = C:/Program Files (x86)/Arduino/hardware/tools/avr/bin/
	JAVAPATH = C:/Program Files/Java/jdk1.7.0_40/bin/
	JAVAC = $(JAVAPATH)javac.exe
	JAVA = $(JAVAPATH)java.exe
else
	UNAME_S := $(shell uname -s)
	UNAME_P := $(shell uname -p)
	UNAME_M := $(shell uname -m)
	ifeq ($(UNAME_S),Linux)
		ifeq ($(UNAME_P),x86_64)
			BINPATH = ../arduino/bin-linux64/bin/
		else ifneq ($(filter %86,$(UNAME_P)),)
			BINPATH = ../arduino/bin-linux32/bin/
		else ifneq ($(filter arm%,$(UNAME_M)),)
			BINPATH = /usr/bin/
		endif
	endif
endif

CFLAGS = -c -g -Os -fno-inline-small-functions -fno-split-wide-types -mshort-calls -Wall -mmcu=$(MCU) -DF_CPU=$(CLK) -std=gnu99
LDFLAGS = -Os -Wl,--section-start=.text=0x7000 -nostartfiles -mmcu=$(MCU) 

all: clean out

clean:
	- rm *.o
	- rm *.elf
	- rm *.hex

$(PROJECT).o: $(PROJECT).c
	"$(BINPATH)avr-gcc" $(CFLAGS) -I$(INCLUDES) $< -o $@

$(PROJECT).elf: $(PROJECT).o
	"$(BINPATH)avr-gcc" $(LDFLAGS) -o $@ $< -lm
	
$(PROJECT).hex: $(PROJECT).elf
	"$(BINPATH)avr-objcopy" -O ihex -R .eeprom $< $@ 

out: $(PROJECT).hex
	"$(BINPATH)avr-size" $(PROJECT).elf

load: clean out isp

isp: $(PROJECT).hex
	"$(BINPATH)avrdude" $(ISP_ARGS) -e -u -Ulock:w:0x3f:m -qq -Uefuse:w:0x$(ISP_EFUSE):m -Uhfuse:w:0x$(ISP_HFUSE):m -Ulfuse:w:0x$(ISP_LFUSE):m -Ulock:w:0x0f:m
	"$(BINPATH)avrdude" $(ISP_ARGS) -V -q -s -Uflash:w:$(PROJECT).hex
	"$(BINPATH)avrdude" $(ISP_ARGS)
