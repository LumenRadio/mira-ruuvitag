#ifndef BOARD_H
#define BOARD_H

#include <mira.h>

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

#if 1
#define BOARD_STDOUT_RTT_ID             0
#endif

void board_setup(
    void);

void board_led_set(
    int num,
    int val);

#endif
