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

#include "board.h"
#include <mira.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app-config.h"
#include "gpiote-nrf-drv.h"
#include "lis2dh12_registers.h"
#include "sensor-lis2dh12.h"
#include "sensors.h"
#include "spi-if.h"

static lis2dh12_scale_t state_scale = LIS2DH12_SCALE2G;
static lis2dh12_resolution_t state_resolution = LIS2DH12_RES10BIT;
static uint8_t movements = 0;
static bool acc_config_updated = false;

process_event_t sensor_lis2dh12_init_done_evt;

PROCESS(sensor_lis2dh12_init, "Sensor: LIS2DH12 init");
PROCESS(sensor_lis2dh12_sample, "Sensor: LIS2DH12 sample");

/****************** SPI TRANSFER FUNCTIONS *********************/
mira_status_t
accel_write_reg(uint8_t reg, uint8_t value)
{
    mira_status_t ret;
    uint8_t tx_buf[2];

    tx_buf[0] = SPI_MASK | reg;
    tx_buf[1] = value;
    ret = mira_spi_transfer(SPI_ID, tx_buf, 2, NULL, 0);
    if (MIRA_SUCCESS != ret) {
        printf("accel_write_reg: error %u while writing register %d\n", ret, reg);
    }
    return ret;
}

mira_status_t
accel_write_reg_len(uint8_t reg, uint8_t* tx_buf, uint8_t tx_buf_len)
{
    mira_status_t ret;
    tx_buf[0] = SPI_MASK | reg | SPI_ADR_INC;

    ret = mira_spi_transfer(SPI_ID, tx_buf, tx_buf_len + 1, NULL, 0);
    if (MIRA_SUCCESS != ret) {
        printf("accel_write_reg: error %u while writing register %d\n", ret, reg);
    }
    return ret;
}

mira_status_t
accel_read_reg(uint8_t reg)
{
    mira_status_t ret;
    uint8_t tx_buf1[1];
    uint8_t rx_buf[8];

    tx_buf1[0] = reg | SPI_READ | SPI_ADR_INC;
    printf("tx: %02x\n", tx_buf1[0]);
    memset(rx_buf, 0x00, 7);
    ret = mira_spi_transfer(SPI_ID, tx_buf1, 1, rx_buf, 7);
    if (MIRA_SUCCESS != ret) {
        printf("Error: %u \n", ret);
        return ret;
    }
    return ret;
}

mira_status_t
accel_read_reg_len(uint8_t reg, uint8_t* rx_buf, uint8_t rx_buff_len)
{
    mira_status_t ret;
    uint8_t tx_buf[1];

    tx_buf[0] = reg | SPI_READ | SPI_ADR_INC;
    ret = mira_spi_transfer(SPI_ID, tx_buf, 1, rx_buf, rx_buff_len + 1);
    if (MIRA_SUCCESS != ret) {
        printf("Error: %u \n", ret);
    }
    return ret;
}

/****************** BIT CONVERSION *********************/

static inline int16_t
lis2dh12_from_fs2_hr_to_mg(int16_t lsb)
{
    return (lsb / 16) * 1;
}

static inline int16_t
lis2dh12_from_fs4_hr_to_mg(int16_t lsb)
{
    return (lsb / 16) * 2;
}

static inline int16_t
lis2dh12_from_fs8_hr_to_mg(int16_t lsb)
{
    return (lsb / 16) * 4;
}

static inline int16_t
lis2dh12_from_fs16_hr_to_mg(int16_t lsb)
{
    return (lsb / 16) * 12;
}

static inline int16_t
lis2dh12_from_fs2_nm_to_mg(int16_t lsb)
{
    return (lsb / 64) * 4;
}

static inline int16_t
lis2dh12_from_fs4_nm_to_mg(int16_t lsb)
{
    return (lsb / 64) * 8;
}

static inline int16_t
lis2dh12_from_fs8_nm_to_mg(int16_t lsb)
{
    return (lsb / 64) * 16;
}

static inline int16_t
lis2dh12_from_fs16_nm_to_mg(int16_t lsb)
{
    return (lsb / 64) * 48;
}

static inline int16_t
lis2dh12_from_fs2_lp_to_mg(int16_t lsb)
{
    return (lsb / 256) * 16;
}

static inline int16_t
lis2dh12_from_fs4_lp_to_mg(int16_t lsb)
{
    return (lsb / 256) * 32;
}

static inline int16_t
lis2dh12_from_fs8_lp_to_mg(int16_t lsb)
{
    return (lsb / 256) * 64;
}

static inline int16_t
lis2dh12_from_fs16_lp_to_mg(int16_t lsb)
{
    return (lsb / 256) * 192;
}

/**
 * Convert raw value to acceleration in mg. Reads scale and resolution from
 * state variables.
 *
 * @param raw_acceleration raw ADC value from LIS2DH12
 * @return int16_t representing acceleration in milli-G.
 */
static int16_t
rawToMg(int16_t raw_acceleration)
{
    switch (state_scale) {
        case LIS2DH12_SCALE2G:
            switch (state_resolution) {
                case LIS2DH12_RES8BIT:
                    return lis2dh12_from_fs2_lp_to_mg(raw_acceleration);

                case LIS2DH12_RES10BIT:
                    return lis2dh12_from_fs2_nm_to_mg(raw_acceleration);

                case LIS2DH12_RES12BIT:
                    return lis2dh12_from_fs2_hr_to_mg(raw_acceleration);

                default:
                    break;
            }
            break;

        case LIS2DH12_SCALE4G:
            switch (state_resolution) {
                case LIS2DH12_RES8BIT:
                    return lis2dh12_from_fs4_lp_to_mg(raw_acceleration);

                case LIS2DH12_RES10BIT:
                    return lis2dh12_from_fs4_nm_to_mg(raw_acceleration);

                case LIS2DH12_RES12BIT:
                    return lis2dh12_from_fs4_hr_to_mg(raw_acceleration);
            }
            break;

        case LIS2DH12_SCALE8G:
            switch (state_resolution) {
                case LIS2DH12_RES8BIT:
                    return lis2dh12_from_fs8_lp_to_mg(raw_acceleration);

                case LIS2DH12_RES10BIT:
                    return lis2dh12_from_fs8_nm_to_mg(raw_acceleration);

                case LIS2DH12_RES12BIT:
                    return lis2dh12_from_fs8_hr_to_mg(raw_acceleration);

                default:
                    break;
            }
            break;

        case LIS2DH12_SCALE16G:
            switch (state_resolution) {
                case LIS2DH12_RES8BIT:
                    return lis2dh12_from_fs16_lp_to_mg(raw_acceleration);

                case LIS2DH12_RES10BIT:
                    return lis2dh12_from_fs16_nm_to_mg(raw_acceleration);

                case LIS2DH12_RES12BIT:
                    return lis2dh12_from_fs16_hr_to_mg(raw_acceleration);

                default:
                    break;
            }
            break;

        default:
            break;
    }
    // reached only in case of an error, return "smallest representable value"
    return 0x8000;
}

/****************** OTHER FUNCTIONS *********************/

static void
int_handler_callback_func(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    movements++;
    printf("Motion Detected, COUNT:%d\n", movements);

    return;
}

void
sensor_lis2dh12_reinit_sensor(void)
{
    acc_config_updated = true;
}

static void
sensor_lis2dh12_config_update_done(void)
{
    acc_config_updated = false;
}

static bool
get_sensor_lis2dh12_config_update_status(void)
{
    return acc_config_updated;
}
/****************** PROCESSES *********************/

PROCESS_THREAD(sensor_lis2dh12_init, ev, data)
{
    static sensor_lis2dh12_ctx_t* ctx;
    static uint8_t rx_buf[10];
    static uint8_t tx_buf[32];

    PROCESS_BEGIN();

    printf("Sensor LIS2DH12 started\n");

    sensor_lis2dh12_init_done_evt = process_alloc_event();
    /* Initiate edge event */
    gpiote_edge_event_init(BOARD_LIS2DH12_INT2_PIN,
                           NRF_GPIOTE_POLARITY_LOTOHI,
                           NRF_GPIO_PIN_NOPULL,
                           int_handler_callback_func);

    ctx = data;
    memset(ctx, 0, sizeof(sensor_lis2dh12_ctx_t));

    /* Self test SPI transfer */
    PROCESS_WAIT_UNTIL(spi_request(BOARD_LIS2DH12_CS_PIN));
    accel_read_reg_len(LIS2DH12_WHO_AM_I, rx_buf, 2);

    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));

    spi_release(BOARD_LIS2DH12_CS_PIN);
    /* END Self test SPI transfer */

    if (rx_buf[1] == LIS2DH12_ID) {
        /* Self test passed */
        printf("ACC sensor available\n");
        ctx->sensor_available = MIRA_TRUE;
        ctx->val_x.type = SENSOR_VALUE_TYPE_ACC_X;
        ctx->val_y.type = SENSOR_VALUE_TYPE_ACC_Y;
        ctx->val_z.type = SENSOR_VALUE_TYPE_ACC_Z;
        ctx->val_move_count.type = SENSOR_VALUE_TYPE_MOVE_COUNT;

        while (1) {

            /* Reset SPI transfer */

            memset(tx_buf, 0x00, sizeof(tx_buf));
            tx_buf[1] = 0x10;
            tx_buf[2] = 0x00;
            tx_buf[3] = 0x07;

            PROCESS_WAIT_UNTIL(spi_request(BOARD_LIS2DH12_CS_PIN));
            accel_write_reg_len(0x1E, &tx_buf[1], 9);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            memset(tx_buf, 0x00, sizeof(tx_buf));
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(0x2E, &tx_buf[2], 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            memset(tx_buf, 0x00, sizeof(tx_buf));
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(0x30, &tx_buf[2], 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            memset(tx_buf, 0x00, sizeof(tx_buf));
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(0x32, &tx_buf[2], 3);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            memset(tx_buf, 0x00, sizeof(tx_buf));
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(0x36, &tx_buf[2], 3);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            memset(tx_buf, 0x00, sizeof(tx_buf));
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(0x3A, &tx_buf[2], 6);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);
            /* END Reset SPI transfer */

            /* Enable XYZ SPI transfer */
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg(LIS2DH12_CTRL_REG1, LIS2DH12_XYZ_EN_MASK);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);
            /* END enable XYZ SPI transfer */

            /* Set scale SPI transfer */
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);

            // Read current value of CTRL4 Register
            static uint8_t ctrl4_2[2] = { 0, 0 };
            accel_read_reg_len(LIS2DH12_CTRL_REG4, ctrl4_2, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            // Reset scale bits
            ctrl4_2[1] &= ~LIS2DH12_FS_MASK;
            ctrl4_2[1] |= LIS2DH12_SCALE2G;

            // Write register value back to lis2dh12
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(LIS2DH12_CTRL_REG4, ctrl4_2, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);
            /* END Set scale SPI transfer */

            /* Set sample rate SPI transfer */
            static uint8_t ctrl[2] = { 0, 0 };

            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_read_reg_len(LIS2DH12_CTRL_REG1, ctrl, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            // Clear sample rate bits
            ctrl[1] &= ~LIS2DH12_ODR_MASK;
            // Setup sample rate
            ctrl[1] |= LIS2DH12_RATE_10;

            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(LIS2DH12_CTRL_REG1, ctrl, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);
            /* END Set sample rate SPI transfer */

            /* Set resolution SPI transfer */
            static uint8_t ctrl1[2] = { 0, 0 };
            static uint8_t ctrl4[2] = { 0, 0 };
            // Read registers 3 & 4
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_read_reg_len(LIS2DH12_CTRL_REG1, ctrl1, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_read_reg_len(LIS2DH12_CTRL_REG4, ctrl4, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            // Reset Low-power, high-resolution masks
            ctrl1[1] &= ~LIS2DH12_LPEN_MASK;
            ctrl4[1] &= ~LIS2DH12_HR_MASK;
            static lis2dh12_resolution_t resolution = LIS2DH12_RES10BIT;

            switch (resolution) {
                case LIS2DH12_RES12BIT:
                    ctrl4[1] |= LIS2DH12_HR_MASK;
                    break;

                // No action needed
                case LIS2DH12_RES10BIT:
                    break;

                case LIS2DH12_RES8BIT:
                    ctrl1[1] |= LIS2DH12_LPEN_MASK;
                    break;

                // Writing normal power to lis2dh12 is safe
                default:
                    break;
            }
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(LIS2DH12_CTRL_REG1, ctrl1, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(LIS2DH12_CTRL_REG4, ctrl4, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);
            /* END Set resolution SPI transfer */

            /*************** START INTERRUPT PIN CONFIG ***************/
            /* START Config for Interrupt function 2 - SPI transfer*/
            static uint8_t cfg[2] = { 0, 0 };

            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_read_reg_len(LIS2DH12_CTRL_REG2, cfg, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            // Enable high-pass for Interrupt function 2.
            cfg[1] |= LIS2DH12_HPIS2_MASK;

            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(LIS2DH12_CTRL_REG2, cfg, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            // Enable interrupt 2 on X-Y-Z HI/LO.
            cfg[1] = 0;
            cfg[1] |= LIS2DH12_6D_MASK | LIS2DH12_ZHIE_MASK | LIS2DH12_ZLIE_MASK;
            cfg[1] |= LIS2DH12_YHIE_MASK | LIS2DH12_YLIE_MASK;
            cfg[1] |= LIS2DH12_XHIE_MASK | LIS2DH12_XLIE_MASK;

            // Setup interrupt configuration: AND/OR of events, X-Y-Z Hi/Lo,
            // 6-direction detection
            static uint8_t int2_cfg[2] = { 0, 0 };
            int2_cfg[1] = cfg[1];
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(LIS2DH12_INT2_CFG, int2_cfg, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);
            /* END Config for Interrupt function 2 - SPI transfer*/

            // Setup number of LSBs needed to trigger activity interrupt.
            static uint8_t int2_ths[2] = { 0, 0 };
            int2_ths[1] = app_config.move_threshold;
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(LIS2DH12_INT2_THS, int2_ths, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_cs_not_active_set(BOARD_LIS2DH12_CS_PIN);

            // Enable Interrupt function 2 on LIS interrupt pin 2 (stays high
            // for 1/ODR).
            static uint8_t ctrl6[2];
            ctrl6[1] |= LIS2DH12_I2C_INT2_MASK;
            spi_cs_active_set(BOARD_LIS2DH12_CS_PIN);
            accel_write_reg_len(LIS2DH12_CTRL_REG6, ctrl6, 1);
            PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
            spi_release(BOARD_LIS2DH12_CS_PIN);

            process_post(&sensors_proc, sensor_lis2dh12_init_done_evt, NULL);

            sensor_lis2dh12_config_update_done();
            PROCESS_YIELD_UNTIL(get_sensor_lis2dh12_config_update_status());
        }
    } else {
        /* Self test not passed */
        printf("ACC sensor not available\n");
        ctx->sensor_available = MIRA_FALSE;
        ctx->val_x.type = SENSOR_VALUE_TYPE_NONE;
        ctx->val_y.type = SENSOR_VALUE_TYPE_NONE;
        ctx->val_z.type = SENSOR_VALUE_TYPE_NONE;
        ctx->val_move_count.type = SENSOR_VALUE_TYPE_NONE;
    }
    PROCESS_END();
}

PROCESS_THREAD(sensor_lis2dh12_sample, ev, data)
{
    static sensor_lis2dh12_ctx_t* ctx;

    PROCESS_BEGIN();

    ctx = data;

    PROCESS_PAUSE();

    static lis2dh12_sensor_buffer_t acc_val;
    static uint8_t read_buffer[10];
    memset(read_buffer, 0x00, sizeof(read_buffer));
    memset(&acc_val, 0x00, sizeof(lis2dh12_sensor_buffer_t));

    /* read out X,Y,Z */
    PROCESS_WAIT_UNTIL(spi_request(BOARD_LIS2DH12_CS_PIN));
    accel_read_reg_len(LIS2DH12_OUT_X_L, read_buffer, sizeof(lis2dh12_sensor_buffer_t));
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_release(BOARD_LIS2DH12_CS_PIN);

    /* remove the first 0xFF */
    memcpy(&acc_val, &(read_buffer[1]), sizeof(lis2dh12_sensor_buffer_t));

    /* Convert to Mg */
    acc_val.sensor.x = rawToMg(acc_val.sensor.x);
    acc_val.sensor.y = rawToMg(acc_val.sensor.y);
    acc_val.sensor.z = rawToMg(acc_val.sensor.z);

    ctx->val_x.value_p = acc_val.sensor.x;
    ctx->val_x.value_q = 1024;

    ctx->val_y.value_p = acc_val.sensor.y;
    ctx->val_y.value_q = 1024;

    ctx->val_z.value_p = acc_val.sensor.z;
    ctx->val_z.value_q = 1024;

    /* Movement counter value */
    ctx->val_move_count.value_p = movements;
    ctx->val_move_count.value_q = 1;

    PROCESS_END();
}
