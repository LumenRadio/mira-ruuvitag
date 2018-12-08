#ifndef SENSOR_BME280_H
#define SENSOR_BME280_H

#include <mira.h>
#include "sensor-bme280-math.h"
#include "sensor-value.h"

typedef struct {
    sensor_bme280_calib_t cal;
    sensor_value_t val_temperature;
    sensor_value_t val_humidity;
    sensor_value_t val_pressure;
} sensor_bme280_ctx_t;

/**
 * Call and wait to finish for sensor to start up
 */
PROCESS_NAME(sensor_bme280_init);

/**
 * Call and wait to finish for sensor to sample data
 */
PROCESS_NAME(sensor_bme280_sample);

#endif
