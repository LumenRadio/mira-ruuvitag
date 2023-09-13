REPODIR := $(shell git rev-parse --show-toplevel)

# For NRF SDK configuration needed for drivers
CFLAGS += -DUSE_APP_CONFIG
CFLAGS += -I$(REPODIR)/sdk_config

# Dummy target, to ensure it's the first one
all:

PROJECT_NAME = mira-demo

TARGET ?= nrf52832ble-os

HW_REVISION ?= v8

LDFLAGS=-lm

SDKDIR = $(CURDIR)/src/vendor/nrf5-sdk
LIBDIR = $(CURDIR)/src/vendor/libmira

# include all the SDK dependencies
CFLAGS += -I$(SDKDIR)/integration/nrfx/legacy
CFLAGS += -I$(SDKDIR)/modules/nrfx/drivers/include
CFLAGS += -I$(SDKDIR)/modules/nrfx/drivers/src
CFLAGS += -I$(SDKDIR)/modules/nrfx
CFLAGS += -I$(SDKDIR)/integration/nrfx
CFLAGS += -I$(SDKDIR)/config/nrf52832/config
CFLAGS += -I$(SDKDIR)/modules/nrfx/mdk
CFLAGS += -I$(SDKDIR)/components/toolchain/cmsis/include
CFLAGS += -I$(SDKDIR)/components/libraries/util
CFLAGS += -I$(SDKDIR)/components/softdevice/s132/headers/
CFLAGS += -I$(SDKDIR)/components/libraries/log
CFLAGS += -I$(SDKDIR)/components/libraries/experimental_section_vars
CFLAGS += -I$(SDKDIR)/components/libraries/log/src
CFLAGS += -I$(SDKDIR)/components/libraries/delay

# include build dependencies
CFLAGS += -I$(BUILD_DIR)/
CFLAGS += -I$(BUILD_DIR)/config

RUUIV_APP_DIR = $(CURDIR)/src/application
BOARDS_DIR = $(CURDIR)/boards
DRIVERS_DIR = $(CURDIR)/src/drivers
SENSORS_DIR = $(CURDIR)/src/sensors

# include ruuvi application dependencies
CFLAGS += -I$(RUUIV_APP_DIR)
CFLAGS += -I$(BOARDS_DIR)
CFLAGS += -I$(DRIVERS_DIR)/nfc
CFLAGS += -I$(DRIVERS_DIR)/nrf-gpiote
CFLAGS += -I$(DRIVERS_DIR)/spi
CFLAGS += -I$(CURDIR)/src/net-status
CFLAGS += -I$(CURDIR)/src/network-metrics
CFLAGS += -I$(SENSORS_DIR)/battery
CFLAGS += -I$(SENSORS_DIR)/bme280
CFLAGS += -I$(SENSORS_DIR)/lis2dh12
CFLAGS += -I$(DRIVERS_DIR)/i2c
CFLAGS += -I$(SENSORS_DIR)/shtc3
CFLAGS += -I$(SENSORS_DIR)/dps310

#Define device
CFLAGS += -DNRF52832_XXAA

#define Ruuvi firmware version
ifeq ($(HW_REVISION), v7)
CFLAGS += -DRUUVI_V_71
else
ifeq ($(HW_REVISION), v8)
CFLAGS += -DRUUVI_V_8
else
$(error HW_REVISION $(HW_REVISION) not recognized)
endif
endif

# include math dependencies
LDFLAGS += -lm
LDFLAGS += -u _printf_float

SOURCE_FILES = \
	$(RUUIV_APP_DIR)/app-config.c \
	$(RUUIV_APP_DIR)/application.c \
	$(RUUIV_APP_DIR)/board.c \
	$(RUUIV_APP_DIR)/boot.c \
	$(CURDIR)/src/net-status/net-status.c \
	$(DRIVERS_DIR)/nfc/nfc-if.c \
	$(SENSORS_DIR)/battery/sensor-battery.c \
	$(SENSORS_DIR)/bme280/sensor-bme280.c \
	$(SENSORS_DIR)/bme280/sensor-bme280-math.c \
	$(RUUIV_APP_DIR)/sensor-value.c \
	$(RUUIV_APP_DIR)/sensors.c \
	$(RUUIV_APP_DIR)/sensors-sender.c \
	$(CURDIR)/src/network-metrics/network-metrics.c \
	$(SENSORS_DIR)/lis2dh12/sensor-lis2dh12.c \
	$(DRIVERS_DIR)/spi/spi-if.c \
	$(DRIVERS_DIR)/nrf-gpiote/gpiote-nrf-drv.c \
	$(DRIVERS_DIR)/i2c/i2c-nrf-drv.c \
	$(SENSORS_DIR)/shtc3/sensor-shtc3.c \
	$(SENSORS_DIR)/dps310/sensor-dps310.c \
	$(SDKDIR)/modules/nrfx/drivers/src/nrfx_gpiote.c \
	$(SDKDIR)/integration/nrfx/legacy/nrf_drv_twi.c \
	$(SDKDIR)/modules/nrfx/drivers/src/nrfx_twim.c

# For NFC
APP_VERSION := $(shell git rev-parse --short HEAD)
CFLAGS += -DAPPLICATION_TARGET=\"$(TARGET)\"
CFLAGS += -DAPPLICATION_VERSION=\"$(APP_VERSION)\"

include $(LIBDIR)/Makefile.include
