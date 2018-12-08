PROJECT_NAME = ruuvimira

TARGET ?= nrf52832-mesh
LIBDIR ?= $(CURDIR)/../mira/build/libmira

SOURCE_FILES = \
	app-config.c \
	application.c \
	board.c \
	boot.c \
	net-status.c \
	nfc-if.c \
	sensor-bme280.c \
	sensor-bme280-math.c \
	spi-if.c

include $(LIBDIR)/Makefile.include
