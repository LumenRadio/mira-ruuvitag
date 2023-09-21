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

#include <mira.h>
#include "sensor-bme280.h"
#include "sensor-bme280-math.h"
#include "spi-if.h"
#include "board.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#define BME280_READ             0x80
#define BME280_WRITE            0x00

#define BME280_REG_RESET        0xE0
#define BME280_RESET_VAL        0xB6
#define BME_280_ID_REG          0xD0
#define BME_280_ID              0x60

PROCESS(sensor_bme280_init, "Sensor: BME280 startup");
PROCESS(sensor_bme280_sample, "Sensor: BME280 reader");

PROCESS_THREAD(
    sensor_bme280_init,
    ev,
    data)
{
    static struct etimer tm;
    static sensor_bme280_ctx_t *ctx;
    static uint8_t tx_buf[2];
    static uint8_t raw88[0xa1 - 0x88 + 1];
    static uint8_t rawe1[0xf0 - 0xe1 + 1];
    static uint8_t bme_id;
    static uint8_t bme_id_buffer[2];

    PROCESS_BEGIN();

    ctx = data;

    memset(ctx, 0, sizeof(sensor_bme280_ctx_t));

    PROCESS_PAUSE();

    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));
    tx_buf[0] = BME280_READ | BME_280_ID_REG;
    mira_spi_transfer(SPI_ID, tx_buf, 1, bme_id_buffer, 2);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    bme_id = bme_id_buffer[1];
    spi_release(BOARD_BME280_CS_PIN);
    bme_id = bme_id_buffer[1];

    if (bme_id == BME_280_ID) {
        printf("BME280 sensor is available\n");
        ctx->sensor_available = MIRA_TRUE;
        ctx->val_temperature.type = SENSOR_VALUE_TYPE_TEMPERATURE;
        ctx->val_pressure.type = SENSOR_VALUE_TYPE_PRESSURE;
        ctx->val_humidity.type = SENSOR_VALUE_TYPE_HUMIDITY;

        PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));
        tx_buf[0] = BME280_WRITE | (BME280_REG_RESET & 0x7f);
        tx_buf[1] = BME280_RESET_VAL;
        mira_spi_transfer(SPI_ID, tx_buf, 2, NULL, 0);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        etimer_set(&tm, CLOCK_SECOND / 8);
        PROCESS_YIELD_UNTIL(etimer_expired(&tm));

        /*
        * Read all registers from 0x81, which means raw_regs[reg-1] will
        * be the register value. raw_regs[0] is undefined
        */
        spi_cs_active_set(BOARD_BME280_CS_PIN);
        tx_buf[0] = BME280_READ | (0x88 & 0x7f);
        mira_spi_transfer(SPI_ID, tx_buf, 1, raw88, (0xa1 - 0x88 + 1));
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        spi_cs_active_set(BOARD_BME280_CS_PIN);
        tx_buf[0] = BME280_READ | (0xe1 & 0x7f);
        mira_spi_transfer(SPI_ID, tx_buf, 1, rawe1, (0xf0 - 0xe1 + 1));
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_release(BOARD_BME280_CS_PIN);

        /* First byte in each buffer is the command */
        sensor_bme280_math_populate_calib(&ctx->cal, raw88 + 1, rawe1 + 1);
    } else {
        printf("BME280 sensor is not avialble\n");
        ctx->sensor_available = MIRA_FALSE;
        ctx->val_temperature.type = SENSOR_VALUE_TYPE_NONE;
        ctx->val_pressure.type = SENSOR_VALUE_TYPE_NONE;
        ctx->val_humidity.type = SENSOR_VALUE_TYPE_NONE;
    }

    PROCESS_END();
}

PROCESS_THREAD(
    sensor_bme280_sample,
    ev,
    data)
{
    static sensor_bme280_ctx_t *ctx;
    static uint8_t tx_buf[6];
    static uint8_t rx_buf[9];
    static struct etimer tm;

    PROCESS_BEGIN();

    ctx = data;

    PROCESS_PAUSE();

    /*
     * Configure sensor and start measurement
     */
    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));

    tx_buf[0] = BME280_WRITE | (0xf2 & 0x7f); /* ctrl_hum */
    tx_buf[1] = (3 << 0); /* osrs_h = oversampling x1 => 1 */

    tx_buf[2] = BME280_WRITE | (0xf4 & 0x7f); /* ctrl_meas */
    tx_buf[3] = (1 << 0) | (1 << 2) | (1 << 5); /* mode = 1 (forced mode), osrs_p = 1, osrs_t = 1 */

    tx_buf[4] = BME280_WRITE | (0xf5 & 0x7f); /* config */
    tx_buf[5] = 0; /* t_sb = dont care, filter = 0 (off), spi3w_en = 0 */

    mira_spi_transfer(SPI_ID, tx_buf, 6, NULL, 0);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_release(BOARD_BME280_CS_PIN);

    /* Wait until done */
    etimer_set(&tm, CLOCK_SECOND / 8);
    PROCESS_YIELD_UNTIL(etimer_expired(&tm));

    /* Read out measurements */
    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));
    tx_buf[0] = BME280_READ | (0xf7 & 0x7f); /* measurement results */
    mira_spi_transfer(SPI_ID, tx_buf, 1, rx_buf, 9);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_release(BOARD_BME280_CS_PIN);

    /* Print measurements */

    int32_t adc_p = ((uint32_t) rx_buf[1]) << 12
        | ((uint32_t) rx_buf[2]) << 4
        | ((uint32_t) rx_buf[3]) >> 4;

    int32_t adc_t = ((uint32_t) rx_buf[4]) << 12
        | ((uint32_t) rx_buf[5]) << 4
        | ((uint32_t) rx_buf[6]) >> 4;

    int32_t adc_h = ((uint32_t) rx_buf[7]) << 8
        | ((uint32_t) rx_buf[8]) << 0;

    int32_t value_T = sensor_bme280_math_calc_t(&ctx->cal, adc_t);
    int32_t value_P = sensor_bme280_math_calc_p(&ctx->cal, adc_p, value_T);
    int32_t value_H = sensor_bme280_math_calc_h(&ctx->cal, adc_h, value_T);

    ctx->val_temperature.value_p = value_T;
    ctx->val_temperature.value_q = 5120;

    ctx->val_pressure.value_p = value_P;
    ctx->val_pressure.value_q = 256;

    ctx->val_humidity.value_p = value_H;
    ctx->val_humidity.value_q = 1024;

    PROCESS_END();
}
