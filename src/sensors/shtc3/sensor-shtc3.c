/*----------------------------------------------------------------------------
Copyright (c) 2021 LumenRadio AB
This code is the property of LumenRadio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.
This example is provided as is, without warranty.
----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "board.h"
#include "i2c-nrf-drv.h"
#include "sensor-shtc3.h"

#define SENSOR_ID 0x0807
#define SENSOR_ID_MASK 0X083F

/*function prototype */
float sensor_sht31_calc_temp(uint16_t raw_value);
float sensor_sht31_calc_humid(uint16_t raw_value);

PROCESS(sensor_shtc3_init, "Sensor init process");
PROCESS(sensor_shtc3_sample, "Sensor smaple process");

PROCESS_THREAD(sensor_shtc3_init, ev, data)
{
    static sensor_shtc3_ctx_t* ctx;
    static i2c_config_t config = { .scl = BOARD_I2C_SCL_PIN, .sda = BOARD_I2C_SDA_PIN };
    static uint8_t read_id[2] = { 0xEF, 0xC8 };
    static volatile uint8_t values[2];
    static uint16_t shtc3_id;
    PROCESS_BEGIN();
    ctx = data;

    i2c_init(&config);

    memset(ctx, 0, sizeof(sensor_shtc3_ctx_t));

    /* Test to see if the sensor exists */
    memset((uint8_t*)values, 0x00, sizeof(values));
    nrf_i2c_read(SHTC3_I2C_SLAVE_ADDRESS, read_id, sizeof(read_id), values, sizeof(values));
    PROCESS_WAIT_EVENT_UNTIL(nrf_i2c_xfer_is_done());

    shtc3_id = (shtc3_id | values[0]) << 8;
    shtc3_id = shtc3_id | values[1];

    if ((shtc3_id & SENSOR_ID_MASK) == SENSOR_ID) {
        printf("SHTC3 sensor is avaialable\n");
        ctx->sensor_available = MIRA_TRUE;
        ctx->val_temperature.type = SENSOR_VALUE_TYPE_TEMPERATURE;
        ctx->val_humidity.type = SENSOR_VALUE_TYPE_HUMIDITY;
    } else {
        printf("SHTC3 sensor is not avaialable\n");
        ctx->sensor_available = MIRA_FALSE;
        ctx->val_temperature.type = SENSOR_VALUE_TYPE_NONE;
        ctx->val_humidity.type = SENSOR_VALUE_TYPE_NONE;
    }

    PROCESS_END();
}

PROCESS_THREAD(sensor_shtc3_sample, ev, data)
{
    static sensor_shtc3_ctx_t* ctx;

    static uint8_t wakeup_config[2] = { 0x35, 0x17 };

    static uint8_t measure_config[2] = { 0x64, 0x58 };
    static uint8_t sleep_command[2] = { 0xB0, 0x98 };

    static volatile uint8_t values[6];

    PROCESS_BEGIN();
    ctx = data;
    PROCESS_PAUSE();

    nrf_i2c_write(SHTC3_I2C_SLAVE_ADDRESS, wakeup_config, sizeof(wakeup_config));
    PROCESS_WAIT_EVENT_UNTIL(nrf_i2c_xfer_is_done());

    nrf_i2c_write(SHTC3_I2C_SLAVE_ADDRESS, wakeup_config, sizeof(wakeup_config));
    PROCESS_WAIT_EVENT_UNTIL(nrf_i2c_xfer_is_done());

    memset((uint8_t*)values, 0x00, sizeof(values));
    nrf_i2c_read(
      SHTC3_I2C_SLAVE_ADDRESS, measure_config, sizeof(measure_config), values, sizeof(values));
    PROCESS_WAIT_EVENT_UNTIL(nrf_i2c_xfer_is_done());

    uint16_t raw_T = ((values[0] << 8) & 0xFFFF) | (values[1] & 0xFF);
    float temp = sensor_sht31_calc_temp(raw_T);
    ctx->val_temperature.value_p = temp * 1024;
    ctx->val_temperature.value_q = 1024;

    uint16_t raw_H = ((values[3] << 8) & 0xFFFF) | (values[4] & 0xFF);
    float RH = sensor_sht31_calc_humid(raw_H);
    ctx->val_humidity.value_p = RH * 1024;
    ctx->val_humidity.value_q = 1024;

    nrf_i2c_write(SHTC3_I2C_SLAVE_ADDRESS, sleep_command, sizeof(sleep_command));
    PROCESS_WAIT_EVENT_UNTIL(nrf_i2c_xfer_is_done());

    nrf_i2c_write(SHTC3_I2C_SLAVE_ADDRESS, sleep_command, sizeof(sleep_command));
    PROCESS_WAIT_EVENT_UNTIL(nrf_i2c_xfer_is_done());

    PROCESS_END();
}

/* Helper functions */
float
sensor_sht31_calc_temp(uint16_t raw_value)
{
    return -45 + 175 * ((float)raw_value / 0x10000);
};

float
sensor_sht31_calc_humid(uint16_t raw_value)
{
    return 100 * ((float)raw_value / 0x10000);
};