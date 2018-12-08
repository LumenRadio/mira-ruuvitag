#include "sensors.h"

#include "sensor-value.h"
#include "sensor-bme280.h"

#include "nfc-if.h"

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
static sensor_bme280_ctx_t sensor_bme280_ctx;

/*
 * List of handlers
 */
static const sensor_value_t *sensor_values[] = {
    &sensor_bme280_ctx.val_temperature,
    &sensor_bme280_ctx.val_humidity,
    &sensor_bme280_ctx.val_pressure,
    NULL
};
static const sensor_handler_t sensor_handler[] = {
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
         * Print all sesnor values
         */
        printf("Sensor values:\n");
        for (i = 0; sensor_values[i] != NULL; i++)
        {
            const sensor_value_t *val = sensor_values[i];
            printf(
                "%16s = %10ld / %10lu %s\n",
                val->name,
                val->value_p,
                val->value_q,
                sensor_value_unit_name[val->unit]);
        }

        etimer_set(&timer, CLOCK_SECOND * 10);
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
        val_pq /= val->value_q;

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
