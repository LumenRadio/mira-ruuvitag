#include "sensor-value.h"

const char *sensor_value_unit_name[] = {
    [SENSOR_VALUE_UNIT_NONE] = "none",
    [SENSOR_VALUE_UNIT_DEG_C] = "deg C",
    [SENSOR_VALUE_UNIT_PASCAL] = "Pa",
    [SENSOR_VALUE_UNIT_PERCENT] = "%"
};


void sensor_value_pack(
    uint8_t *buf,
    const sensor_value_t *val)
{

}
