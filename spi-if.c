#include <mira.h>
#include "spi-if.h"
#include "board.h"

static int spi_is_locked = 0;
process_event_t spi_release_event;

const static mira_spi_config_t spi_config = {
    .frequency = 250000,
    .sck_pin = BOARD_SPI_SCK_PIN,
    .mosi_pin = BOARD_SPI_MOSI_PIN,
    .miso_pin = BOARD_SPI_MISO_PIN,
    .ss_pin = MIRA_GPIO_PIN_UNDEFINED,
    .mode = BOARD_SPI_MODE,
    .bit_order = MIRA_BIT_ORDER_MSB_FIRST
};

void spi_init(
    void)
{
    spi_release_event = process_alloc_event();
}

int spi_request(
    mira_gpio_pin_t spi_cs)
{
    if (spi_is_locked)
    {
        return 0;
    }
    mira_spi_init(0, &spi_config);
    mira_gpio_set_value(spi_cs, MIRA_FALSE);
    spi_is_locked = 1;
    return 1;
}

void spi_release(
    mira_gpio_pin_t spi_cs)
{
    mira_gpio_set_value(spi_cs, MIRA_TRUE);
    mira_spi_uninit(0);
    spi_is_locked = 0;
    process_post(PROCESS_BROADCAST, spi_release_event, NULL);
}
