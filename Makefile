PROJECT_NAME = ruuvimira

TARGET ?= nrf52832-mesh
LIBDIR ?= $(CURDIR)/../mira/build/libmira

SOURCE_FILES = \
	app-config.c \
	application.c \
	boot.c \
	net-status.c \
	nfc-if.c

include $(LIBDIR)/Makefile.include
