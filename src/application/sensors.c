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

#include "sensors.h"

#include "sensor-value.h"

#include "network-metrics.h"
#include "sensor-battery.h"
#include "sensor-bme280.h"
#include "sensor-dps310.h"
#include "sensor-lis2dh12.h"
#include "sensor-shtc3.h"

#include "sensors-sender.h"

#include "app-config.h"
#include "board.h"
#include "nfc-if.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
    struct process* init_proc;
    struct process* sample_proc;
    void* storage;
    mira_bool_t* sensor_available;
    process_event_t* init_done_evnt;
} sensor_handler_t;

/*
 * Sensors
 */
#if BATTERY_ENABLED
static sensor_battery_ctx_t sensor_battery_ctx;
#endif
#if BME280_ENABLED
static sensor_bme280_ctx_t sensor_bme280_ctx;
#endif
#if NETWORK_METRICS_ENABLED
static network_metrics_ctx_t network_metrics_ctx;
#endif
#if ACCELEROMETER_ENABLED
static sensor_lis2dh12_ctx_t sensor_lis2dh12_ctx;
#endif
#if SHTC3_ENABLED
static sensor_shtc3_ctx_t sensor_shtc3_ctx;
#endif
#if DPS310_ENABLED
static sensor_dps310_ctx_t sensor_dps310_ctx;
#endif

/*
 * List of handlers
 */
static const sensor_value_t* sensor_values[] = {
#if BATTERY_ENABLED
    &sensor_battery_ctx.val_battery,
#endif
#if SHTC3_ENABLED
    &sensor_shtc3_ctx.val_temperature,
    &sensor_shtc3_ctx.val_humidity,
#endif
#if DPS310_ENABLED
    &sensor_dps310_ctx.val_pressure,
#endif
#if BME280_ENABLED
    &sensor_bme280_ctx.val_temperature,
    &sensor_bme280_ctx.val_humidity,
    &sensor_bme280_ctx.val_pressure,
#endif
#if NETWORK_METRICS_ENABLED
    &network_metrics_ctx.etx,
#endif
#if ACCELEROMETER_ENABLED
    &sensor_lis2dh12_ctx.val_x,
    &sensor_lis2dh12_ctx.val_y,
    &sensor_lis2dh12_ctx.val_z,
    &sensor_lis2dh12_ctx.val_move_count,
#endif
    NULL
};

static const sensor_handler_t sensor_handler[] = {
#if BATTERY_ENABLED
    { &sensor_battery_init,
      &sensor_battery_sample,
      &sensor_battery_ctx,
      &sensor_battery_ctx.sensor_available,
      NULL },
#endif
#if SHTC3_ENABLED
    { &sensor_shtc3_init,
      &sensor_shtc3_sample,
      &sensor_shtc3_ctx,
      &sensor_shtc3_ctx.sensor_available,
      NULL },
#endif
#if DPS310_ENABLED
    { &sensor_dps310_init,
      &sensor_dps310_sample,
      &sensor_dps310_ctx,
      &sensor_dps310_ctx.sensor_available,
      NULL },
#endif
#if BME280_ENABLED
    { &sensor_bme280_init,
      &sensor_bme280_sample,
      &sensor_bme280_ctx,
      &sensor_bme280_ctx.sensor_available,
      NULL },
#endif
#if NETWORK_METRICS_ENABLED
    { &network_metrics_init,
      &network_metrics_sample,
      &network_metrics_ctx,
      &network_metrics_ctx.sensor_available,
      NULL },
#endif
#if ACCELEROMETER_ENABLED
    { &sensor_lis2dh12_init,
      &sensor_lis2dh12_sample,
      &sensor_lis2dh12_ctx,
      &sensor_lis2dh12_ctx.sensor_available,
      &sensor_lis2dh12_init_done_evt },
#endif
    { NULL, NULL, NULL, NULL },
};

/*
 * NFC interface
 */
static void sensors_nfc_on_open(mira_nfc_ndef_writer_t* writer);

static const nfcif_handler_t sensors_nfc_handler = { .on_open = sensors_nfc_on_open };

/*
 * UDP interface
 */
static sensors_sender_context_t sender_ctx;

/*
 * Processes
 */
PROCESS(sensors_proc, "Sensors");

void
sensors_init(void)
{
    process_start(&sensors_proc, NULL);
}

PROCESS_THREAD(sensors_proc, ev, data)
{
    static int i;
    static int sensor_count;
    static struct etimer timer;
    PROCESS_BEGIN();

    printf("Sensors: Starting\n");
    /*
     * Start all sensors
     */
    for (i = 0; sensor_handler[i].init_proc != NULL; i++) {
        process_start(sensor_handler[i].init_proc, sensor_handler[i].storage);
        PROCESS_WAIT_EVENT_UNTIL(ev == *(sensor_handler[i].init_done_evnt) ||
                                 !(process_is_running(sensor_handler[i].init_proc)));
    }

    printf("Sensors: Started\n");

    /* Enable NFC first when sensors are started */
    nfcif_register_handler(&sensors_nfc_handler);
    sensors_sender_init(&sender_ctx);

    /*
     * Start polling
     */
    while (1) {
        /*
         * Sample all sensors
         */
        for (i = 0; sensor_handler[i].init_proc != NULL; i++) {
            if (*sensor_handler[i].sensor_available == MIRA_TRUE) {
                process_start(sensor_handler[i].sample_proc, sensor_handler[i].storage);
            }
        }

        /*
         * Wait for all sensors to have sampled data
         */
        for (i = 0; sensor_handler[i].init_proc != NULL; i++) {
            if (*sensor_handler[i].sensor_available == MIRA_TRUE) {
                PROCESS_WAIT_WHILE(process_is_running(sensor_handler[i].sample_proc));
            }
        }

        /*
         * Print and count sensor values
         */
        printf("Sensor values:\n");
        sensor_count = 0;
        for (i = 0; sensor_values[i] != NULL; i++) {
            if (sensor_values[i]->type != SENSOR_VALUE_TYPE_NONE) {
                const sensor_value_t* val = sensor_values[i];
                printf("%16s = %10ld / %10lu %s\n",
                       sensor_value_type_name[val->type],
                       val->value_p,
                       val->value_q,
                       sensor_value_unit_name[val->type]);

                sensor_count++;
            }
        }

        /*
         * Send sensor values to root
         */
        sensors_sender_send(&sender_ctx, sensor_values, sensor_count);

        etimer_set(&timer, CLOCK_SECOND * app_config.update_interval);

        PROCESS_YIELD_UNTIL(etimer_expired(&timer));
    }

    PROCESS_END();
}

static void
sensors_nfc_on_open(mira_nfc_ndef_writer_t* writer)
{
    int i;
    char vendor[128];
    char value[128];
    for (i = 0; sensor_values[i] != NULL; i++) {
        const sensor_value_t* val = sensor_values[i];
        int64_t val_pq;

        int32_t val_int;  /* Integer values */
        int32_t val_fraq; /* Fractional values, thousands */

        val_pq = val->value_p;
        val_pq *= 1000;
        if (val->value_q != 0) {
            val_pq /= val->value_q;
        }

        val_int = val_pq / 1000;
        val_fraq = val_pq % 1000;

        if (val_fraq < 0) {
            val_fraq += 1000;
        }

        sprintf(vendor,
                "application/vnd.lumenradio.sensor.%s",
                sensor_value_type_name[sensor_values[i]->type]);
        sprintf(value, "%ld.%03ld %s", val_int, val_fraq, sensor_value_unit_name[val->type]);

        mira_nfc_ndef_write_copy(writer,
                                 MIRA_NFC_NDEF_TNF_MIME_TYPE,
                                 (const uint8_t*)vendor,
                                 strlen(vendor),
                                 NULL,
                                 0,
                                 (uint8_t*)value,
                                 strlen(value));
    }
}
