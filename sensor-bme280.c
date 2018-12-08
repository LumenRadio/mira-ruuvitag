#include <mira.h>
#include "sensor-bme280.h"
#include "sensor-bme280-math.h"
#include "nfc-if.h"
#include "spi-if.h"
#include "board.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static sensor_bme280_calib_t bme280_calib;

#define BME280_READ             0x80
#define BME280_WRITE            0x00

#define BME280_REG_RESET        0xE0
#define BME280_RESET_VAL        0xB6

PROCESS(sensor_bme280_startup, "Sensor: BME280 startup");
PROCESS(sensor_bme280_reader, "Sensor: BME280 reader");
PROCESS(sensor_bme280_connection, "Sensor: BME280");

static void sensor_bme280_init_nfc_on_open(
    mira_nfc_ndef_writer_t *writer)
{
}

void sensor_bme280_init(
    void)
{
    process_start(&sensor_bme280_connection, NULL);

    nfcif_register_handler(&(nfcif_handler_t ) {
            .on_open = sensor_bme280_init_nfc_on_open
        });
}

PROCESS_THREAD(
    sensor_bme280_startup,
    ev,
    data)
{
    static struct etimer tm;
    static uint8_t tx_buf[2];
    static uint8_t raw88[0xa1-0x88+1];
    static uint8_t rawe1[0xf0-0xe1+1];

    PROCESS_BEGIN();

    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));
    tx_buf[0] = BME280_WRITE | (BME280_REG_RESET & 0x7f);
    tx_buf[1] = BME280_RESET_VAL;
    mira_spi_transfer(0, tx_buf, 2, NULL, 0);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(0));
    spi_release(BOARD_BME280_CS_PIN);

    etimer_set(&tm, CLOCK_SECOND / 8);
    PROCESS_YIELD_UNTIL(etimer_expired(&tm));


    /*
     * Read all registers from 0x81, which means raw_regs[reg-1] will
     * be the register value. raw_regs[0] is undefined
     */
    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));
    tx_buf[0] = BME280_READ | (0x88 & 0x7f);
    mira_spi_transfer(0, tx_buf, 1, raw88, (0xa1-0x88+1));
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(0));
    spi_release(BOARD_BME280_CS_PIN);

    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));
    tx_buf[0] = BME280_READ | (0xe1 & 0x7f);
    mira_spi_transfer(0, tx_buf, 1, rawe1, (0xf0-0xe1+1));
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(0));
    spi_release(BOARD_BME280_CS_PIN);

    /* First byte in each buffer is the command */
    sensor_bme280_math_populate_calib(&bme280_calib, raw88+1, rawe1+1);

    PROCESS_END();
}


PROCESS_THREAD(
    sensor_bme280_reader,
    ev,
    data)
{
    static uint8_t tx_buf[6];
    static uint8_t rx_buf[9];
    static struct etimer tm;
    PROCESS_BEGIN();

    /*
     * Configure sensor and start measurement
     */
    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));

    tx_buf[0] = BME280_WRITE | (0xf2 & 0x7f); /* ctrl_hum */
    tx_buf[1] = (3<<0); /* osrs_h = oversampling x1 => 1 */

    tx_buf[2] = BME280_WRITE | (0xf4 & 0x7f); /* ctrl_meas */
    tx_buf[3] = (1<<0) | (1<<2) | (1<<5); /* mode = 1 (forced mode), osrs_p = 1, osrs_t = 1 */

    tx_buf[4] = BME280_WRITE | (0xf5 & 0x7f); /* config */
    tx_buf[5] = 0; /* t_sb = dont care, filter = 0 (off), spi3w_en = 0 */

    mira_spi_transfer(0, tx_buf, 6, NULL, 0);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(0));
    spi_release(BOARD_BME280_CS_PIN);


    /* Wait until done */
    etimer_set(&tm, CLOCK_SECOND / 8);
    PROCESS_YIELD_UNTIL(etimer_expired(&tm));

    /* Read out measurements */
    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));
    tx_buf[0] = BME280_READ | (0xf7 & 0x7f); /* measurement results */
    mira_spi_transfer(0, tx_buf, 1, rx_buf, 9);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(0));
    spi_release(BOARD_BME280_CS_PIN);


    /* Print measurements */

    int32_t adc_p = ((uint32_t)rx_buf[1]) << 12
                  | ((uint32_t)rx_buf[2]) << 4
                  | ((uint32_t)rx_buf[3]) >> 4;

    int32_t adc_t = ((uint32_t)rx_buf[4]) << 12
                  | ((uint32_t)rx_buf[5]) << 4
                  | ((uint32_t)rx_buf[6]) >> 4;

    int32_t adc_h = ((uint32_t)rx_buf[7]) << 8
                  | ((uint32_t)rx_buf[8]) << 0;

    int32_t val_t;
    uint32_t val_p;
    uint32_t val_h;
    int32_t t_fine;

    t_fine = sensor_bme280_math_calc_tfine(&bme280_calib, adc_t);
    val_t = sensor_bme280_math_calc_t(t_fine);
    val_p = sensor_bme280_math_calc_p(&bme280_calib, adc_p, t_fine);
    val_h = sensor_bme280_math_calc_h(&bme280_calib, adc_h, t_fine);

    printf("T=%ld P=%lu H=%f\n", val_t, val_p, val_h);

    PROCESS_END();
}

PROCESS_THREAD(
    sensor_bme280_connection,
    ev,
    data)
{
    static struct etimer tm;

    PROCESS_BEGIN();

    process_start(&sensor_bme280_startup, NULL);
    PROCESS_WAIT_WHILE(process_is_running(&sensor_bme280_startup));

    printf("bme280 started\n");

    while(1) {
        etimer_set(&tm, CLOCK_SECOND * 2);

        process_start(&sensor_bme280_reader, NULL);
        PROCESS_WAIT_WHILE(process_is_running(&sensor_bme280_reader));

        PROCESS_YIELD_UNTIL(etimer_expired(&tm));
    }

    PROCESS_END();
}
