#ifndef SENSOR_BATTERY_H
#define SENSOR_BATTERY_H

#include <mira.h>
#include "sensor-value.h"

typedef struct {
    mira_adc_context_t adc;
    sensor_value_t val_battery;
} sensor_battery_ctx_t;

/**
 * Call and wait to finish for sensor to start up
 */
PROCESS_NAME(sensor_battery_init);

/**
 * Call and wait to finish for sensor to sample data
 */
PROCESS_NAME(sensor_battery_sample);

#endif
