#include <mira.h>
#include "board.h"

#include "spi-if.h"
#include "nfc-if.h"

MIRA_IODEFS(
    MIRA_IODEF_NONE,
#if defined(BOARD_STDOUT_RTT_ID)
    MIRA_IODEF_RTT(BOARD_STDOUT_RTT_ID),
#elif defined(BOARD_STDOUT_UART_ID)
    MIRA_IODEF_UART(BOARD_STDOUT_UART_ID),
#else
    MIRA_IODEF_NONE,
#endif
    MIRA_IODEF_NONE,
    );

void board_setup(
    void)
{
    uint32_t i;


#if defined(BOARD_STDOUT_RTT_ID)
    mira_rtt_init();
    mira_rtt_set_blocking(BOARD_STDOUT_RTT_ID, 1);
#endif

#if defined(BOARD_STDOUT_UART_ID)
    mira_uart_init(BOARD_STDOUT_UART_ID, &(mira_uart_config_t){
        .tx_pin = BOARD_UART_PIN_TX,
        .rx_pin = BOARD_UART_PIN_RX,
        .baudrate = BOARD_UART_BAUDRATE
    });
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
    mira_gpio_set_dir(BOARD_LIS2DH12_INT1_PIN, MIRA_GPIO_DIR_IN);
    mira_gpio_set_dir(BOARD_LIS2DH12_INT1_PIN, MIRA_GPIO_DIR_IN);

    /* Pull both CS low to enable SPI mode on devices, set high afterwards */
    mira_gpio_set_value(BOARD_BME280_CS_PIN, MIRA_FALSE);
    mira_gpio_set_value(BOARD_LIS2DH12_CS_PIN, MIRA_FALSE);

    for(i=0; i<1000000; i++) { asm volatile ("nop"); }

    /* Set both CS high, which is idle state */
    mira_gpio_set_value(BOARD_BME280_CS_PIN, MIRA_TRUE);
    mira_gpio_set_value(BOARD_LIS2DH12_CS_PIN, MIRA_TRUE);

    spi_init();
    nfcif_init();
}

void board_led_set(
    int num,
    int val)
{
    mira_gpio_set_value(
        num == 1 ? BOARD_LED1_PIN : BOARD_LED2_PIN,
        val ? MIRA_FALSE : MIRA_TRUE);
}
