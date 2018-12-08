#ifndef SENSOR_VALUE_H
#define SENSOR_VALUE_H

#include <stdint.h>

/**
 * Unit of a value
 */
typedef enum {
    SENSOR_VALUE_UNIT_NONE = 0, /* No value set */
    SENSOR_VALUE_UNIT_DEG_C,
    SENSOR_VALUE_UNIT_PASCAL,
    SENSOR_VALUE_UNIT_PERCENT,
    SENSOR_VALUE_UNIT_VOLT
} sensor_value_unit_t;

/**
 * Storage of a sensor value, to easily handled over the air
 *
 * The value is sent as a rational value, name and a unit for easy
 * presentation
 */
typedef struct {
    int32_t value_p;
    uint32_t value_q;
    sensor_value_unit_t unit;
    char name[16];
} sensor_value_t;

#define SENSOR_VALUE_PACKED_SIZE 16

void sensor_value_pack(uint8_t *buf, const sensor_value_t *val);

extern const char *sensor_value_unit_name[];

#endif
