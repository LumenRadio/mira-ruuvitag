/*----------------------------------------------------------------------------
Copyright (c) 2021 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
----------------------------------------------------------------------------*/

#include "i2c-nrf-drv.h"
#include <stdbool.h>
#include <mira.h>

#define DEBUG 0

#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* (DEBUG) */

/* TWI instance ID. */
#define TWI_INSTANCE_ID     1

static bool i2c_is_initialized = false;

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

/* Twi transfer indicators. */
static volatile bool m_xfer_done = false;

static struct process *waiting_process;

static void twi_handler(
    nrf_drv_twi_evt_t const *p_event,
    void *p_context)
{
    switch (p_event->type) {
    case NRF_DRV_TWI_EVT_DONE:
        m_xfer_done = true;
        process_poll(waiting_process);
        if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_TX) {
            PRINTF("TX done\n");
        } else if (p_event->xfer_desc.type == NRF_DRV_TWI_XFER_RX) {
            PRINTF("RX done\n");
        } else {
            PRINTF("done\n");
        }
        break;

    default:
        PRINTF("Other event: %d\n", p_event->type);
        break;
    }
}

int nrf_i2c_xfer_is_done()
{
    return m_xfer_done;
}

int i2c_init (
    i2c_config_t *config)
{

    if (!i2c_is_initialized) {

        const nrf_drv_twi_config_t twi_config = {
            .scl = config->scl,
            .sda = config->sda,
            .frequency = NRF_DRV_TWI_FREQ_100K,
            .interrupt_priority = APP_IRQ_PRIORITY_LOW,
            .clear_bus_init = false
        };

        uint32_t err_code =
            nrf_drv_twi_init(&m_twi, &twi_config, twi_handler, NULL);
        nrf_drv_twi_enable(&m_twi);

        i2c_is_initialized = true;
        return (err_code == NRF_SUCCESS ? 0 : -1);
    }
    return -1;
}

bool i2c_is_init()
{
    return i2c_is_initialized;
}

int nrf_i2c_write(
    uint8_t I2C_slave_address,
    uint8_t *tx_buff,
    uint8_t len)
{
    m_xfer_done = false;
    waiting_process = PROCESS_CURRENT();
    uint32_t err_code = nrf_drv_twi_tx(&m_twi,
        I2C_slave_address,
        tx_buff,
        len,
        false);

    switch (err_code) {
    case NRF_SUCCESS:
        return 0;

    case NRFX_ERROR_BUSY:
        return 2;

    case NRFX_ERROR_INTERNAL:
        return 3;

    default:
        return -1;
        break;
    }

}

/* Use this with clock stretching only */
int nrf_i2c_read(
    uint8_t I2C_slave_address,
    uint8_t *tx_buff,
    uint8_t tx_len,
    volatile uint8_t *rx_buff,
    uint8_t rx_len)
{
    m_xfer_done = false;
    waiting_process = PROCESS_CURRENT();

    nrf_drv_twi_xfer_desc_t p_xfer = {
        .type = NRF_DRV_TWI_XFER_TXRX,
        .address = I2C_slave_address,
        .primary_length = tx_len,
        .secondary_length = rx_len,
        .p_primary_buf = tx_buff,
        .p_secondary_buf = (uint8_t *) rx_buff
    };

    uint32_t err_code = nrf_drv_twi_xfer(&m_twi, &p_xfer, 0);

    switch (err_code) {
    case NRF_SUCCESS:
        return 0;

    case NRFX_ERROR_BUSY:
        return 2;

    case NRFX_ERROR_INTERNAL:
        return 3;

    default:
        return -1;
        break;
    }

}