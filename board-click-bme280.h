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

#ifndef BOARD_CLICK_H
#define BOARD_CLICK_H

/**
 * If used with nRF52832dk and CLICK boards for sensors to emulate RuuviTag
 */


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

/* 
    *  Enable sensors 
*/

#define BATTERY_ENABLED 1
#define BME280_ENABLED 1
#define NETWORK_METRICS_ENABLED 1

#endif