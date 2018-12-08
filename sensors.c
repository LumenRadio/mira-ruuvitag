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

#include "sensor-battery.h"
#include "sensor-bme280.h"

#include "sensors-sender.h"

#include "nfc-if.h"
#include "app-config.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
    struct process *init_proc;
    struct process *sample_proc;
    void *storage;
} sensor_handler_t;

/*
 * Sensors
 */
static sensor_battery_ctx_t sensor_battery_ctx;
static sensor_bme280_ctx_t sensor_bme280_ctx;

/*
 * List of handlers
 */
static const sensor_value_t *sensor_values[] = {
    &sensor_battery_ctx.val_battery,
    &sensor_bme280_ctx.val_temperature,
    &sensor_bme280_ctx.val_humidity,
    &sensor_bme280_ctx.val_pressure,
    NULL
};
static const sensor_handler_t sensor_handler[] = {
    { &sensor_battery_init, &sensor_battery_sample, &sensor_battery_ctx },
    { &sensor_bme280_init, &sensor_bme280_sample, &sensor_bme280_ctx },
    { NULL, NULL, NULL },
};

/*
 * NFC interface
 */
static void sensors_nfc_on_open(
    mira_nfc_ndef_writer_t *writer);

static const nfcif_handler_t sensors_nfc_handler = {
    .on_open = sensors_nfc_on_open
};

/*
 * UDP interface
 */
static sensors_sender_context_t sender_ctx;

/*
 * Processes
 */
PROCESS(sensors_proc, "Sensors");

void sensors_init(
    void)
{
    process_start(&sensors_proc, NULL);
}

PROCESS_THREAD(
    sensors_proc,
    ev,
    data)
{
    static int i;
    static int sensor_count;
    static struct etimer timer;
    PROCESS_BEGIN();

    printf("Sensors: Starting\n");
    /*
     * Start all sensors
     */
    for (i = 0; sensor_handler[i].init_proc != NULL; i++)
    {
        process_start(sensor_handler[i].init_proc, sensor_handler[i].storage);
    }

    /*
     * Wait for all sesnors to finish starting up
     */
    for (i = 0; sensor_handler[i].init_proc != NULL; i++)
    {
        PROCESS_WAIT_WHILE(process_is_running(sensor_handler[i].init_proc));
    }

    printf("Sensors: Started\n");

    /* Enable NFC first when sensors are started */
    nfcif_register_handler(&sensors_nfc_handler);
    sensors_sender_init(&sender_ctx);

    /*
     * Start polling
     */
    while (1)
    {
        /*
         * Sample all sensors
         */
        for (i = 0; sensor_handler[i].init_proc != NULL; i++)
        {
            process_start(
                sensor_handler[i].sample_proc,
                sensor_handler[i].storage);
        }

        /*
         * Wait for all sensors to have sampled data
         */
        for (i = 0; sensor_handler[i].init_proc != NULL; i++)
        {
            PROCESS_WAIT_WHILE(
                process_is_running(sensor_handler[i].sample_proc));
        }

        /*
         * Print and count sesnor values
         */
        printf("Sensor values:\n");
        sensor_count = 0;
        for (i = 0; sensor_values[i] != NULL; i++)
        {
            const sensor_value_t *val = sensor_values[i];
            printf(
                "%16s = %10ld / %10lu %s\n",
                val->name,
                val->value_p,
                val->value_q,
                sensor_value_unit_name[val->unit]);
            sensor_count++;
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

static void sensors_nfc_on_open(
    mira_nfc_ndef_writer_t *writer)
{
    int i;
    char vendor[128];
    char value[128];
    for (i = 0; sensor_values[i] != NULL; i++)
    {
        const sensor_value_t *val = sensor_values[i];
        int64_t val_pq;

        int32_t val_int; /* Integer values */
        int32_t val_fraq; /* Fractional values, thousands */

        val_pq = val->value_p;
        val_pq *= 1000;
        if(val->value_q != 0) {
            val_pq /= val->value_q;
        }

        val_int = val_pq / 1000;
        val_fraq = val_pq % 1000;

        if(val_fraq < 0) {
            val_fraq += 1000;
        }

        sprintf(
            vendor,
            "application/vnd.lumenradio.sesnor.%s",
            sensor_values[i]->name);
        sprintf(
            value,
            "%ld.%03ld %s",
            val_int,
            val_fraq,
            sensor_value_unit_name[val->unit]);

        mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
            (const uint8_t *) vendor, strlen(vendor),
            NULL, 0,
            (uint8_t *) value, strlen(value)
                );
    }
}
