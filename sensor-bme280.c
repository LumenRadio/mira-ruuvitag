#include <mira.h>
#include "sensor-bme280.h"
#include "nfc-if.h"
#include "spi-if.h"
#include "board.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    uint8_t dig_H1;
    uint8_t dig_H3;
    int16_t dig_H2;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} sensor_bme280_calib_t;

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
#if 0
// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
int32_t t_fine;
int32_t BME280_compensate_T_int32(
    int32_t adc_T)
{
    int32_t var1, var2, T;
    var1 = ((((adc_T >> 3) – ((int32_t)bme280_calib.dig_T1<<1))) * ((int32_t)bme280_calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) – ((int32_t)bme280_calib.dig_T1)) * ((adc_T>>4) – ((int32_t)bme280_calib.dig_T1))) >> 12) * ((int32_t)bme280_calib.dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}
// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
uint32_t BME280_compensate_P_int64(
    int32_t adc_P)
{
    int64_t var1, var2, p;
    var1 = ((int64_t) t_fine) – 128000;
    var2 = var1 * var1 * (int64_t) bme280_calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t) bme280_calib.dig_P5) << 17);
    var2 = var2 + (((int64_t) bme280_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t) bme280_calib.dig_P3) >> 8) + ((var1 * (int64_t) bme280_calib.dig_P2) << 12);
    var1 = (((((int64_t) 1) << 47) + var1)) * ((int64_t) bme280_calib.dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t) bme280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t) bme280_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t) bme280_calib.dig_P7) << 4);
    return (uint32_t) p;
}
// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
// Output value of “47445” represents 47445/1024 = 46.333 %RH
uint32_t bme280_compensate_H_int32(
    int32_t adc_H)
{
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine – ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) – (((int32_t)bme280_calib.dig_H4) << 20) – (((int32_t)bme280_calib.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)bme280_calib.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)bme280_calib.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)bme280_calib.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r – (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)bme280_calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t) (v_x1_u32r >> 12);
}
#endif
PROCESS_THREAD(
    sensor_bme280_startup,
    ev,
    data)
{
    static struct etimer tm;
    static uint8_t tx_buf[2];
    static uint8_t raw[128];

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
    tx_buf[0] = BME280_READ | (0x81 & 0x7f);
    mira_spi_transfer(0, tx_buf, 1, raw, 128);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(0));
    spi_release(BOARD_BME280_CS_PIN);

    bme280_calib.dig_T1 = (uint16_t) raw[0x08] | ((uint16_t) raw[0x09]) << 8;
    bme280_calib.dig_T2 = (int16_t) raw[0x0a] | ((uint16_t) raw[0x0b]) << 8;
    bme280_calib.dig_T3 = (int16_t) raw[0x0c] | ((uint16_t) raw[0x0d]) << 8;
    bme280_calib.dig_P1 = (uint16_t) raw[0x0e] | ((uint16_t) raw[0x0f]) << 8;
    bme280_calib.dig_P2 = (int16_t) raw[0x10] | ((int16_t) raw[0x11]) << 8;
    bme280_calib.dig_P3 = (int16_t) raw[0x12] | ((int16_t) raw[0x13]) << 8;
    bme280_calib.dig_P4 = (int16_t) raw[0x14] | ((int16_t) raw[0x15]) << 8;
    bme280_calib.dig_P5 = (int16_t) raw[0x16] | ((int16_t) raw[0x17]) << 8;
    bme280_calib.dig_P6 = (int16_t) raw[0x18] | ((int16_t) raw[0x19]) << 8;
    bme280_calib.dig_P7 = (int16_t) raw[0x1a] | ((int16_t) raw[0x1b]) << 8;
    bme280_calib.dig_P8 = (int16_t) raw[0x1c] | ((int16_t) raw[0x1d]) << 8;
    bme280_calib.dig_P9 = (int16_t) raw[0x1e] | ((int16_t) raw[0x1f]) << 8;
    bme280_calib.dig_H1 = (uint8_t) raw[0x21];
    bme280_calib.dig_H2 = (int16_t) raw[0x61] | ((int16_t) raw[0x62]) << 8;
    bme280_calib.dig_H3 = (uint8_t) raw[0x63];
    bme280_calib.dig_H4 = ((uint16_t) raw[0x65] & 0x0f)
        | ((int16_t) (int8_t) raw[0x64]) << 4;
    bme280_calib.dig_H5 = ((uint16_t) (raw[0x65] & 0xf0) >> 4)
        | ((int16_t) (int8_t) raw[0x66]) << 4;
    bme280_calib.dig_H6 = (int8_t) raw[0x67];


    PROCESS_END();
}


PROCESS_THREAD(
    sensor_bme280_reader,
    ev,
    data)
{
    static uint8_t tx_buf[6];
    static uint8_t rx_buf[9];
    static int i;
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
#if 0
    do {
        PROCESS_PAUSE();
        PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));

        tx_buf[0] = BME280_READ | (0xf3 & 0x7f); /* status */

        mira_spi_transfer(0, tx_buf, 1, rx_buf, 2);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(0));
        spi_release(BOARD_BME280_CS_PIN);
    } while(rx_buf[1] != 0);
#endif

    /* Read out measurements */
    PROCESS_WAIT_UNTIL(spi_request(BOARD_BME280_CS_PIN));

    tx_buf[0] = BME280_READ | (0xf7 & 0x7f); /* measurement results */

    mira_spi_transfer(0, tx_buf, 1, rx_buf, 9);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(0));
    spi_release(BOARD_BME280_CS_PIN);

    /* Print measurements */
    printf("Measurements:");
    for(i=0; i<8; i++) {
        printf(" %02x", rx_buf[i+1]);
    }
    printf("\n");

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

    printf(" Cal T1: %d\n", (int) bme280_calib.dig_T1);
    printf(" Cal T2: %d\n", (int) bme280_calib.dig_T2);
    printf(" Cal T3: %d\n", (int) bme280_calib.dig_T3);
    printf(" Cal P1: %d\n", (int) bme280_calib.dig_P1);
    printf(" Cal P2: %d\n", (int) bme280_calib.dig_P2);
    printf(" Cal P3: %d\n", (int) bme280_calib.dig_P3);
    printf(" Cal P4: %d\n", (int) bme280_calib.dig_P4);
    printf(" Cal P5: %d\n", (int) bme280_calib.dig_P5);
    printf(" Cal P6: %d\n", (int) bme280_calib.dig_P6);
    printf(" Cal P7: %d\n", (int) bme280_calib.dig_P7);
    printf(" Cal P8: %d\n", (int) bme280_calib.dig_P8);
    printf(" Cal P9: %d\n", (int) bme280_calib.dig_P9);
    printf(" Cal H1: %d\n", (int) bme280_calib.dig_H1);
    printf(" Cal H2: %d\n", (int) bme280_calib.dig_H2);
    printf(" Cal H3: %d\n", (int) bme280_calib.dig_H3);
    printf(" Cal H4: %d\n", (int) bme280_calib.dig_H4);
    printf(" Cal H5: %d\n", (int) bme280_calib.dig_H5);
    printf(" Cal H6: %d\n", (int) bme280_calib.dig_H6);

    while(1) {
        etimer_set(&tm, CLOCK_SECOND * 2);

        process_start(&sensor_bme280_reader, NULL);
        PROCESS_WAIT_WHILE(process_is_running(&sensor_bme280_reader));

        PROCESS_YIELD_UNTIL(etimer_expired(&tm));
    }

    PROCESS_END();
}
