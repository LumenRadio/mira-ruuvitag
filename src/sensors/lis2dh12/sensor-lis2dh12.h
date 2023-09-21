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

#ifndef SENSOR_LIS2DH12_H
#define SENSOR_LIS2DH12_H

#include <mira.h>
#include <stdint.h>
#include "lis2dh12_registers.h"
#include "sensor-value.h"

/* Use the nRF SDK for the interrupt handler */
#undef MIN
#undef MAX

/* TYPES ******************************************************************************************/
/** Structure containing sensor data for all 3 axis */
typedef struct __attribute__((packed)){
    int16_t x;
    int16_t y;
    int16_t z;
} acceleration_t;

/** Union to split raw data to values for each axis */
typedef union {
    uint8_t raw[sizeof(acceleration_t)];
    acceleration_t sensor;
} lis2dh12_sensor_buffer_t;

typedef struct {
    sensor_value_t val_x;
    sensor_value_t val_y;
    sensor_value_t val_z;
    sensor_value_t val_move_count;
    mira_bool_t sensor_available;
} sensor_lis2dh12_ctx_t;

/* Available scales */
typedef enum {
    LIS2DH12_SCALE2G = LIS2DH12_FS_2G, /**< Scale Selection: +/- 2g */
    LIS2DH12_SCALE4G = LIS2DH12_FS_4G,  /**< Scale Selection: +/- 4g */
    LIS2DH12_SCALE8G = LIS2DH12_FS_8G,  /**< Scale Selection: +/- 8g */
    LIS2DH12_SCALE16G = LIS2DH12_FS_16G /**< Scale Selection: +/- 16g */
} lis2dh12_scale_t;

/** Available sample rates */
typedef enum {
    LIS2DH12_RATE_0   = 0,              /**< Power down */
    LIS2DH12_RATE_1   = 1 << 4, /**< 1 Hz */
    LIS2DH12_RATE_10  = 2 << 4, /**< 10 Hz*/
    LIS2DH12_RATE_25  = 3 << 4,
    LIS2DH12_RATE_50  = 4 << 4,
    LIS2DH12_RATE_100 = 5 << 4,
    LIS2DH12_RATE_200 = 6 << 4,
    LIS2DH12_RATE_400 = 7 << 4 /** 1k+ rates not implemented */
} lis2dh12_sample_rate_t;

typedef enum {
    LIS2DH12_RES8BIT = 8,       /**< 8 extra bits */
    LIS2DH12_RES10BIT = 6,              /**< 6 extra bits */
    LIS2DH12_RES12BIT = 4       /**< 4 extra bits */
} lis2dh12_resolution_t;

void sensor_lis2dh12_reinit_sensor(
    void);
/*
    Init LIS2DH12 sensor
    */
PROCESS_NAME(sensor_lis2dh12_init);

/*
    Sample LIS2DH12 sensor
    */
PROCESS_NAME(sensor_lis2dh12_sample);

extern process_event_t sensor_lis2dh12_init_done_evt;

#endif