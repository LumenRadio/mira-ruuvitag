/*----------------------------------------------------------------------------
Copyright (c) 2021 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/

#ifndef I2C_NRF_DRV_H
#define I2C_NRF_DRV_H

#undef MIN
#undef MAX
#include "nrf_drv_twi.h"

typedef struct
{
    uint32_t sda;
    uint32_t scl;
} i2c_config_t;

int i2c_init(i2c_config_t* config);

bool i2c_is_init(void);

int nrf_i2c_write(uint8_t I2C_slave_address, uint8_t* tx_buff, uint8_t len);

/* Use this with clock stretching */
int nrf_i2c_read(uint8_t I2C_slave_address,
                 uint8_t* tx_buff,
                 uint8_t tx_len,
                 volatile uint8_t* rx_buff,
                 uint8_t rx_len);

int nrf_i2c_xfer_is_done(void);

#endif
