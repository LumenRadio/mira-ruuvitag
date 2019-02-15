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

typedef enum {
    SENSOR_VALUE_TYPE_NONE = 0, /* No value set */
    SENSOR_VALUE_TYPE_TEMPERATURE,
    SENSOR_VALUE_TYPE_PRESSURE,
    SENSOR_VALUE_TYPE_HUMIDITY,
    SENSOR_VALUE_TYPE_BATTERY,
    SENSOR_VALUE_TYPE_ETX,
    SENSOR_VALUE_TYPE_CLOCK_DRIFT
} sensor_value_type_t;

/**
 * Storage of a sensor value, to easily handled over the air
 *
 * The value is sent as a rational value, name and a unit for easy
 * presentation
 */
// old
/*
typedef struct {
    int32_t value_p;
    uint32_t value_q;
    sensor_value_unit_t unit;
    char name[16];
} sensor_value_t;
*/


typedef struct {
    int32_t value_p;
    uint32_t value_q;
    sensor_value_type_t type;
} sensor_value_t;

extern const char *sensor_value_unit_name[];
extern const char *sensor_value_type_name[];

#endif
