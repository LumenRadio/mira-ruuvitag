/*
 * Copyright (c) 2018, LumenRadio AB All rights reserved.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "spi-if.h"
#include "board.h"
#include <mira.h>

static int spi_is_locked = 0;
process_event_t spi_release_event;

const static mira_spi_config_t spi_config = { .frequency = 250000,
                                              .sck_pin = BOARD_SPI_SCK_PIN,
                                              .mosi_pin = BOARD_SPI_MOSI_PIN,
                                              .miso_pin = BOARD_SPI_MISO_PIN,
                                              .ss_pin = MIRA_GPIO_PIN_UNDEFINED,
                                              .mode = BOARD_SPI_MODE,
                                              .bit_order = MIRA_BIT_ORDER_MSB_FIRST };

void
spi_init(void)
{
    spi_release_event = process_alloc_event();
}

int
spi_request(mira_gpio_pin_t spi_cs)
{
    if (spi_is_locked) {
        return 0;
    }
    mira_spi_init(SPI_ID, &spi_config);
    mira_gpio_set_value(spi_cs, MIRA_FALSE);
    spi_is_locked = 1;
    return 1;
}

void
spi_release(mira_gpio_pin_t spi_cs)
{
    mira_gpio_set_value(spi_cs, MIRA_TRUE);
    mira_spi_uninit(SPI_ID);
    spi_is_locked = 0;
    process_post(PROCESS_BROADCAST, spi_release_event, NULL);
}

void
spi_cs_active_set(mira_gpio_pin_t spi_cs)
{
    mira_gpio_set_value(spi_cs, MIRA_FALSE);
}

void
spi_cs_not_active_set(mira_gpio_pin_t spi_cs)
{
    mira_gpio_set_value(spi_cs, MIRA_TRUE);
}
