#include "sensors.h"

#include "sensor-value.h"
#include "sensor-bme280.h"

#include <stdio.h>

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
