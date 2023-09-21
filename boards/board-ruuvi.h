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

#ifndef BOARD_RUUVI_H
#define BOARD_RUUVI_H

#define RUUVI_DEV_BOARD                             0
/*
    *  Enable sensors
*/
/* Common sensors to different versions of Ruuvi */
#define BATTERY_ENABLED                             1
#define NETWORK_METRICS_ENABLED                     1
/* This is only the default value, can be changed with NFC field. Has to be 1, to ensure the setup is completed */
#define ACCELEROMETER_ENABLED                       1

/* Version 71 */
# ifdef RUUVI_V_71
#define BME280_ENABLED                              1
#define SHTC3_ENABLED                               0
#define DPS310_ENABLED                              0
#endif
/* Version 8 */
#ifdef RUUVI_V_8
#define BME280_ENABLED                              0
#define SHTC3_ENABLED                               1
#define DPS310_ENABLED                              1
#endif

#if RUUVI_DEV_BOARD /* Ruuvi tag with dev-board */

#define BOARD_SPI_MOSI_PIN              MIRA_GPIO_PIN(0, 25)
#define BOARD_SPI_MISO_PIN              MIRA_GPIO_PIN(0, 28)
#define BOARD_SPI_SCK_PIN               MIRA_GPIO_PIN(0, 29)
#define BOARD_SPI_MODE                  MIRA_SPI_MODE_3

#define BOARD_BME280_CS_PIN             MIRA_GPIO_PIN(0, 3)

#define BOARD_DPS310_CS_PIN             MIRA_GPIO_PIN(0, 3)

#define BOARD_LIS2DH12_CS_PIN           MIRA_GPIO_PIN(0, 8)
#define BOARD_LIS2DH12_INT1_PIN         MIRA_GPIO_PIN(0, 2)
#define BOARD_LIS2DH12_INT2_PIN         MIRA_GPIO_PIN(0, 6)

#define NRF_GPIO_PIN_MAP(port, pin)     (((port) << 5) | ((pin) & 0x1F))
#define BOARD_I2C_SCL_PIN               NRF_GPIO_PIN_MAP(0, 5)
#define BOARD_I2C_SDA_PIN               NRF_GPIO_PIN_MAP(0, 4)

#define BOARD_LED1_PIN                  MIRA_GPIO_PIN(0, 17)
#define BOARD_LED2_PIN                  MIRA_GPIO_PIN_UNDEFINED

#define BOARD_BUTTON_PIN                MIRA_GPIO_PIN(0, 13)

#define BOARD_UART_PIN_TX               MIRA_GPIO_PIN(0, 19)
#define BOARD_UART_PIN_RX               MIRA_GPIO_PIN(0, 31)

#define BOARD_STDOUT_UART_ID            0
#define BOARD_UART_BAUDRATE             115200

#else /* Regular Ruuvi-tag */

#define BOARD_SPI_MOSI_PIN              MIRA_GPIO_PIN(0, 25)
#define BOARD_SPI_MISO_PIN              MIRA_GPIO_PIN(0, 28)
#define BOARD_SPI_SCK_PIN               MIRA_GPIO_PIN(0, 29)
#define BOARD_SPI_MODE                  MIRA_SPI_MODE_3

#define BOARD_BME280_CS_PIN             MIRA_GPIO_PIN(0, 3)

#define BOARD_DPS310_CS_PIN             MIRA_GPIO_PIN(0, 3)

#define BOARD_LIS2DH12_CS_PIN           MIRA_GPIO_PIN(0, 8)
#define BOARD_LIS2DH12_INT1_PIN         MIRA_GPIO_PIN(0, 2)
#define BOARD_LIS2DH12_INT2_PIN         MIRA_GPIO_PIN(0, 6)

#define NRF_GPIO_PIN_MAP(port, pin)     (((port) << 5) | ((pin) & 0x1F))
#define BOARD_I2C_SCL_PIN               NRF_GPIO_PIN_MAP(0, 5)
#define BOARD_I2C_SDA_PIN               NRF_GPIO_PIN_MAP(0, 4)

#define BOARD_LED1_PIN                  MIRA_GPIO_PIN(0, 17)
#define BOARD_LED2_PIN                  MIRA_GPIO_PIN(0, 19)

#define BOARD_BUTTON_PIN                MIRA_GPIO_PIN(0, 13)

#define BOARD_UART_PIN_TX               MIRA_GPIO_PIN_UNDEFINED
#define BOARD_UART_PIN_RX               MIRA_GPIO_PIN_UNDEFINED

#define SENSOR_PWR_1                    MIRA_GPIO_PIN(0, 7)
#define SENSOR_PWR_2                    MIRA_GPIO_PIN(0, 12)

#endif

#endif
