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

#include "board.h"
#include <mira.h>

#include "nfc-if.h"
#include "spi-if.h"

MIRA_IODEFS(MIRA_IODEF_NONE,
#if defined(BOARD_STDOUT_RTT_ID)
            MIRA_IODEF_RTT(BOARD_STDOUT_RTT_ID),
#elif defined(BOARD_STDOUT_UART_ID)
            MIRA_IODEF_UART(BOARD_STDOUT_UART_ID),
#else
            MIRA_IODEF_NONE,
#endif
            MIRA_IODEF_NONE, );

void
board_setup(void)
{
    uint32_t i;

#if defined(BOARD_STDOUT_RTT_ID)
    mira_rtt_init();
    // mira_rtt_set_blocking(BOARD_STDOUT_RTT_ID, 1);
#endif

#if defined(BOARD_STDOUT_UART_ID)
    mira_uart_init(BOARD_STDOUT_UART_ID,
                   &(mira_uart_config_t){ .tx_pin = BOARD_UART_PIN_TX,
                                          .rx_pin = BOARD_UART_PIN_RX,
                                          .baudrate = BOARD_UART_BAUDRATE });
#endif

    /* Power down sensors and wait
     * as they might still be powered from before a reset
     */
#ifdef SENSOR_PWR_1
    mira_gpio_set_dir(SENSOR_PWR_1, MIRA_GPIO_DIR_OUT);
    mira_gpio_set_value(SENSOR_PWR_1, MIRA_FALSE);
#endif
#ifdef SENSOR_PWR_2
    mira_gpio_set_dir(SENSOR_PWR_2, MIRA_GPIO_DIR_OUT);
    mira_gpio_set_value(SENSOR_PWR_2, MIRA_FALSE);
#endif

    for (i = 0; i < 1000000; i++) {
        asm volatile("nop");
    }

    /* Power up sensors again */
#ifdef SENSOR_PWR_1
    mira_gpio_set_value(SENSOR_PWR_1, MIRA_TRUE);
#endif
#ifdef SENSOR_PWR_2
    mira_gpio_set_value(SENSOR_PWR_2, MIRA_TRUE);
#endif

    /*
     * Enable buttons/LEDs
     */
    mira_gpio_set_dir(BOARD_LED1_PIN, MIRA_GPIO_DIR_OUT);
    mira_gpio_set_dir(BOARD_LED2_PIN, MIRA_GPIO_DIR_OUT);
    mira_gpio_set_value(BOARD_LED1_PIN, MIRA_TRUE);
    mira_gpio_set_value(BOARD_LED2_PIN, MIRA_TRUE);

    mira_gpio_set_dir(BOARD_BUTTON_PIN, MIRA_GPIO_DIR_IN);

    /* Enable SPI pins */

    mira_gpio_set_dir(BOARD_SPI_MOSI_PIN, MIRA_GPIO_DIR_OUT);
    mira_gpio_set_dir(BOARD_SPI_MISO_PIN, MIRA_GPIO_DIR_IN);
    mira_gpio_set_dir(BOARD_SPI_SCK_PIN, MIRA_GPIO_DIR_OUT);

    mira_gpio_set_value(BOARD_SPI_MOSI_PIN, MIRA_TRUE);
    mira_gpio_set_value(BOARD_SPI_SCK_PIN, MIRA_TRUE);

    /* Enable Sensor specific pins to sensors */

    mira_gpio_set_dir(BOARD_BME280_CS_PIN, MIRA_GPIO_DIR_OUT);

    mira_gpio_set_dir(BOARD_LIS2DH12_CS_PIN, MIRA_GPIO_DIR_OUT);

    /* Pull both CS low to enable SPI mode on devices, set high afterwards */
    mira_gpio_set_value(BOARD_BME280_CS_PIN, MIRA_FALSE);
    mira_gpio_set_value(BOARD_LIS2DH12_CS_PIN, MIRA_FALSE);

    for (i = 0; i < 1000000; i++) {
        asm volatile("nop");
    }

    /* Set both CS high, which is idle state */
    mira_gpio_set_value(BOARD_BME280_CS_PIN, MIRA_TRUE);
    mira_gpio_set_value(BOARD_LIS2DH12_CS_PIN, MIRA_TRUE);

    spi_init();
    nfcif_init();
}

void
board_led_set(int num, int val)
{
    mira_gpio_set_value(num == 1 ? BOARD_LED1_PIN : BOARD_LED2_PIN, val ? MIRA_FALSE : MIRA_TRUE);
}
