#ifndef SENSOR_BME280_MATH_H
#define SENSOR_BME280_MATH_H

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

/**
 * Fill calibration struct from calibration registers
 */
void sensor_bme280_math_populate_calib(
    sensor_bme280_calib_t *cal,
    uint8_t *regs88,
    uint8_t *regse1);


/**
 * Calculate temperature
 *
 * Returns the temperature in fine resolution for internal use
 */
int32_t sensor_bme280_math_calc_tfine(
    sensor_bme280_calib_t *cal,
    int32_t adc_T);

/**
 * Calculate temperature from t_fine
 *
 * Returns temperature in 1/25600 deg C
 */
int32_t sensor_bme280_math_calc_t(
    int32_t t_fine);

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa

/**
 * Calculate pressure
 *
 * Returns the pressure in resolution of 1/256 Pa
 */
uint32_t sensor_bme280_math_calc_p(
    sensor_bme280_calib_t *cal,
    int32_t adc_P,
    int32_t t_fine);


/**
 * Calculate humidity
 *
 * Returns the relative humidity in resolution of 1/1024 %RH
 */
uint32_t sensor_bme280_math_calc_h(
    sensor_bme280_calib_t *cal,
    int32_t adc_H,
    int32_t t_fine);

#endif
