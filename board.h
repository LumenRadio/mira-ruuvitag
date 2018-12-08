#ifndef BOARD_H
#define BOARD_H

#include <mira.h>

/**
 * If used with nRF52832dk and CLICK boards for sensors to emulate RuuviTag
 */
#define CLICK_BOARD 0

#if CLICK_BOARD

#define BOARD_SPI_MOSI_PIN              MIRA_GPIO_PIN(0, 23)
#define BOARD_SPI_MISO_PIN              MIRA_GPIO_PIN(0, 24)
#define BOARD_SPI_SCK_PIN               MIRA_GPIO_PIN(0, 25)
#define BOARD_SPI_MODE                  MIRA_SPI_MODE_3

#define BOARD_BME280_CS_PIN             MIRA_GPIO_PIN(0, 22)

#define BOARD_LIS2DH12_CS_PIN           MIRA_GPIO_PIN(0, 20)
#define BOARD_LIS2DH12_INT1_PIN         MIRA_GPIO_PIN(0, 14)
#define BOARD_LIS2DH12_INT2_PIN         MIRA_GPIO_PIN(0, 16)

#define BOARD_LED1_PIN                  MIRA_GPIO_PIN(0, 17)
#define BOARD_LED2_PIN                  MIRA_GPIO_PIN(0, 19)

#define BOARD_BUTTON_PIN                MIRA_GPIO_PIN(0, 13)

#define BOARD_UART_PIN_TX               MIRA_GPIO_PIN(0, 6)
#define BOARD_UART_PIN_RX               MIRA_GPIO_PIN(0, 8)
#define BOARD_UART_BAUDRATE             115200

#define BOARD_STDOUT_UART_ID            0
#else

#define BOARD_SPI_MOSI_PIN              MIRA_GPIO_PIN(0, 25)
#define BOARD_SPI_MISO_PIN              MIRA_GPIO_PIN(0, 28)
#define BOARD_SPI_SCK_PIN               MIRA_GPIO_PIN(0, 29)
#define BOARD_SPI_MODE                  MIRA_SPI_MODE_3

#define BOARD_BME280_CS_PIN             MIRA_GPIO_PIN(0, 3)

#define BOARD_LIS2DH12_CS_PIN           MIRA_GPIO_PIN(0, 8)
#define BOARD_LIS2DH12_INT1_PIN         MIRA_GPIO_PIN(0, 2)
#define BOARD_LIS2DH12_INT2_PIN         MIRA_GPIO_PIN(0, 6)

#define BOARD_LED1_PIN                  MIRA_GPIO_PIN(0, 17)
#define BOARD_LED2_PIN                  MIRA_GPIO_PIN(0, 19)

#define BOARD_BUTTON_PIN                MIRA_GPIO_PIN(0, 13)

#define BOARD_UART_PIN_TX               MIRA_GPIO_PIN_UNDEFINED
#define BOARD_UART_PIN_RX               MIRA_GPIO_PIN_UNDEFINED

//#define BOARD_STDOUT_RTT_ID             0
#endif

void board_setup(
    void);

void board_led_set(
    int num,
    int val);

#endif
