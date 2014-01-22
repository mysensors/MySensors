# -*- makefile -*-

#TARGET = spi-test-adc
#TARGET = spi-test-rtc
TARGET = spi-test-masterslave

OBJECTS = $(TARGET).o spi.o
CFLAGS = -I../libs
include ./Makefile.mf

DEVICE     = at90usb162
CLOCK      = 16000000

examples:
	make -k TARGET=spi-test-adc clean all
	make -k TARGET=spi-test-rtc OBJECTS="spi-test-rtc.o spi.o ds1305.o" clean all
	make -k TARGET=spi-test-masterslave clean all