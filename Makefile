PROJECT_NAME = ruuvimira

TARGET ?= nrf52832ble-os
LIBDIR ?= $(CURDIR)/../mira/build/libmira

SOURCE_FILES = \
	app-config.c \
	application.c \
	board.c \
	boot.c \
	net-status.c \
	nfc-if.c \
	sensor-battery.c \
	sensor-bme280.c \
	sensor-bme280-math.c \
	sensor-value.c \
	sensors.c \
	sensors-sender.c \
	spi-if.c \
	network-metrics.c

include $(LIBDIR)/Makefile.include
