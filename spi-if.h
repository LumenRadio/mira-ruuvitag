#ifndef SPI_H
#define SPI_H

#include <mira.h>

extern process_event_t spi_release_event;

void spi_init(
    void);

int spi_request(
    mira_gpio_pin_t spi_cs);

void spi_release(
    mira_gpio_pin_t spi_cs);

#endif
