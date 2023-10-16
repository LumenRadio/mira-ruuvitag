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

#include "dps310_registers.h"
#include "sensor-dps310.h"
#include "spi-if.h"

#define SCALE_FACTOR 7864320 // Precision and Oversampling rate = 8 times in application

static uint8_t cal_c0[20] = { 0 };
static float last_temperature;
static int32_t temp, prs;
static int32_t T_raw, P_raw;

PROCESS(sensor_dps310_init, "Sensor: DPS310 init");
PROCESS(sensor_dps310_sample, "Sensor: DPS310 sample");

/****************** SPI TRANSFER FUNCTIONS *********************/
mira_status_t
DPS_write_reg_len(uint8_t reg, uint8_t* tx_buf, uint8_t tx_buf_len)
{
    mira_status_t ret;
    tx_buf[0] = DPS310_WRITE | reg;

    ret = mira_spi_transfer(SPI_ID, tx_buf, tx_buf_len + 1, NULL, 0);
    if (MIRA_SUCCESS != ret) {
        printf("DPS_write_reg: error %u while writing register %d\n", ret, reg);
    }
    return ret;
}

mira_status_t
DPS_read_reg_len(uint8_t reg, uint8_t* rx_buf, uint8_t rx_buff_len)
{
    mira_status_t ret;
    uint8_t tx_buf[1];

    tx_buf[0] = DPS310_READ | reg;
    ret = mira_spi_transfer(SPI_ID, tx_buf, 1, rx_buf, rx_buff_len + 1);
    if (MIRA_SUCCESS != ret) {
        printf("Error: %u \n", ret);
    }
    return ret;
}

static const int32_t
getTwosComplement(int32_t raw, uint8_t length)
{
    if (raw & ((uint32_t)1 << (length - 1))) {
        raw -= (uint32_t)1 << length;
    }
    return raw;
}

static void
readcoeffs(sensor_dps310_calib_t* cal)
{
    uint8_t buffer[18];
    memcpy(buffer, cal_c0 + 1, sizeof(buffer));

    // compose coefficients from buffer content
    cal->m_c0Half = ((uint32_t)buffer[0] << 4) | (((uint32_t)buffer[1] >> 4) & 0x0F);
    cal->m_c0Half = getTwosComplement(cal->m_c0Half, 12);
    // c0 is only used as c0*0.5, so c0_half is calculated immediately
    cal->m_c0Half = cal->m_c0Half / 2;

    // now do the same thing for all other coefficients
    cal->m_c1 = (((uint32_t)buffer[1] & 0x0F) << 8) | (uint32_t)buffer[2];
    cal->m_c1 = getTwosComplement(cal->m_c1, 12);
    cal->m_c00 = ((uint32_t)buffer[3] << 12) | ((uint32_t)buffer[4] << 4) |
                 (((uint32_t)buffer[5] >> 4) & 0x0F);
    cal->m_c00 = getTwosComplement(cal->m_c00, 20);
    cal->m_c10 =
      (((uint32_t)buffer[5] & 0x0F) << 16) | ((uint32_t)buffer[6] << 8) | (uint32_t)buffer[7];
    cal->m_c10 = getTwosComplement(cal->m_c10, 20);

    cal->m_c01 = ((uint32_t)buffer[8] << 8) | (uint32_t)buffer[9];
    cal->m_c01 = getTwosComplement(cal->m_c01, 16);

    cal->m_c11 = ((uint32_t)buffer[10] << 8) | (uint32_t)buffer[11];
    cal->m_c11 = getTwosComplement(cal->m_c11, 16);
    cal->m_c20 = ((uint32_t)buffer[12] << 8) | (uint32_t)buffer[13];
    cal->m_c20 = getTwosComplement(cal->m_c20, 16);
    cal->m_c21 = ((uint32_t)buffer[14] << 8) | (uint32_t)buffer[15];
    cal->m_c21 = getTwosComplement(cal->m_c21, 16);
    cal->m_c30 = ((uint32_t)buffer[16] << 8) | (uint32_t)buffer[17];
    cal->m_c30 = getTwosComplement(cal->m_c30, 16);
}

float
sensor_dps310_calc_temp(sensor_dps310_calib_t* cal, int32_t raw_value)
{
    float T_raw_sc;
    float T_comp;

    T_raw_sc = (float)raw_value / SCALE_FACTOR;
    last_temperature = T_raw_sc;
    T_comp = cal->m_c0Half + (cal->m_c1 * T_raw_sc);

    return T_comp;
}

float
sensor_dps310_calc_pres(sensor_dps310_calib_t* cal, int32_t raw_value)
{
    float P_raw_sc;
    float P_comp;

    P_raw_sc = (float)raw_value / SCALE_FACTOR;
    P_comp = cal->m_c00 +
             P_raw_sc * (cal->m_c10 + P_raw_sc * (cal->m_c20 + P_raw_sc * cal->m_c30)) +
             last_temperature * (cal->m_c01 + P_raw_sc * (cal->m_c11 + P_raw_sc * cal->m_c21));

    return P_comp;
}

/****************** PROCESSES *********************/

PROCESS_THREAD(sensor_dps310_init, ev, data)
{

    static sensor_dps310_ctx_t* ctx;
    static uint8_t tx_buf[32];

    PROCESS_BEGIN();
    ctx = data;

    memset(ctx, 0, sizeof(sensor_dps310_ctx_t));

    PROCESS_PAUSE();

    /* Test to see if the sensor exists */
    static uint8_t sensor_id[2];
    PROCESS_WAIT_UNTIL(spi_request(BOARD_DPS310_CS_PIN));
    DPS_read_reg_len(DPS310_REVISION_ID_REG, sensor_id, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_release(BOARD_DPS310_CS_PIN);

    if (sensor_id[1] == DPS310_ID) {
        printf("DPS310 sensor is available\n");
        ctx->sensor_available = MIRA_TRUE;
        ctx->val_pressure.type = SENSOR_VALUE_TYPE_PRESSURE;

        /* Reading the Sensor Operating Mode and Status */
        static uint8_t mode[2] = { 0, 0 };
        PROCESS_WAIT_UNTIL(spi_request(BOARD_DPS310_CS_PIN));
        DPS_read_reg_len(MEAS_CFG, mode, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Reading calibration coefficients */

        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        memset(cal_c0, 0x00, 20);
        DPS_read_reg_len(CAL_C0, cal_c0, 20);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);
        readcoeffs(&ctx->cal); // Function to read calibration coefficients

        /* Read and calculate temperature*/

        /* Temperature measurment configuration - SPI transfer */
        memset(tx_buf, 0x00, sizeof(tx_buf));
        tx_buf[1] = 0xb3;
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Read TMP_CFG*/
        static uint8_t tmp_cfg[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(TMP_CFG, tmp_cfg, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Writing the Sensor Operating Mode and Status - Temperature */
        memset(tx_buf, 0x00, sizeof(tx_buf));
        tx_buf[1] = CMD_TMP;
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_write_reg_len(MEAS_CFG, tx_buf, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Read MEAS_CFG - Temperature*/
        static uint8_t read_mode_T[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(MEAS_CFG, read_mode_T, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Read temperature value - TMP_B2, TMP_B1, TMP_B0 */
        static uint8_t tmp_b2[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(TMP_B2, tmp_b2, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        static uint8_t tmp_b1[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(TMP_B1, tmp_b1, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        static uint8_t tmp_b0[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(TMP_B0, tmp_b0, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        temp = ((uint32_t)tmp_b2[1] << 16) | ((uint32_t)tmp_b1[1] << 8) | ((uint32_t)tmp_b0[1]);
        T_raw = getTwosComplement(temp, 24);
        sensor_dps310_calc_temp(&ctx->cal, T_raw);

        /* Read and calculate pressure */

        /* Presure measurment configuration - SPI transfer */
        memset(tx_buf, 0x00, sizeof(tx_buf));
        tx_buf[1] = PM_RATE_8 | PM_PRC_8;
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_write_reg_len(PRS_CFG, tx_buf, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Read PRS_CFG*/
        static uint8_t prs_cfg[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(PRS_CFG, prs_cfg, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Writing the Sensor Operating Mode and Status */
        memset(tx_buf, 0x00, sizeof(tx_buf));
        tx_buf[1] = CMD_PRS;
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_write_reg_len(MEAS_CFG, tx_buf, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Read MEAS_CFG */
        static uint8_t read_mode_P[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(MEAS_CFG, read_mode_P, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        /* Read pressure value - PRS_B2, PRS_B1, PRS_B0 */
        static uint8_t prs_b2[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(PRS_B2, prs_b2, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        static uint8_t prs_b1[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(PRS_B1, prs_b1, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

        static uint8_t prs_b0[2] = { 0, 0 };
        spi_cs_active_set(BOARD_DPS310_CS_PIN);
        DPS_read_reg_len(PRS_B0, prs_b0, 1);
        PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
        spi_release(BOARD_DPS310_CS_PIN);

        prs = (prs_b2[1] << 16) | (prs_b1[1] << 8) | (prs_b0[1]);
        P_raw = getTwosComplement(prs, 24);
        sensor_dps310_calc_pres(&ctx->cal, P_raw);

    } else {
        printf("DPS310 sensor is not available\n");
        ctx->sensor_available = MIRA_FALSE;
        ctx->val_pressure.type = SENSOR_VALUE_TYPE_NONE;
    }

    PROCESS_END();
}

PROCESS_THREAD(sensor_dps310_sample, ev, data)
{
    static uint8_t tx_buf[32];
    static sensor_dps310_ctx_t* ctx;

    PROCESS_BEGIN();
    ctx = data;
    PROCESS_PAUSE();

    /* Reading the Sensor Operating Mode and Status */
    static uint8_t mode[2] = { 0, 0 };
    PROCESS_WAIT_UNTIL(spi_request(BOARD_DPS310_CS_PIN));
    DPS_read_reg_len(MEAS_CFG, mode, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Reading calibration coefficients */

    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    memset(cal_c0, 0x00, 20);
    DPS_read_reg_len(CAL_C0, cal_c0, 20);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);
    readcoeffs(&ctx->cal); // Function to read calibration coefficients

    /* Read and calculate temperature*/

    /* Temperature measurment configuration - SPI transfer */
    memset(tx_buf, 0x00, sizeof(tx_buf));
    tx_buf[1] = TMP_EXT | TMP_RATE_8 | TMP_PRC_8;
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_write_reg_len(TMP_CFG, tx_buf, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Read TMP_CFG*/
    static uint8_t tmp_cfg[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(TMP_CFG, tmp_cfg, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));

    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Writing the Sensor Operating Mode and Status - Temperature */
    memset(tx_buf, 0x00, sizeof(tx_buf));
    tx_buf[1] = CMD_TMP;
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_write_reg_len(MEAS_CFG, tx_buf, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Read MEAS_CFG - Temperature*/
    static uint8_t read_mode_T[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(MEAS_CFG, read_mode_T, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Read temperature value - TMP_B2, TMP_B1, TMP_B0 */
    static uint8_t tmp_b2[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(TMP_B2, tmp_b2, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    static uint8_t tmp_b1[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(TMP_B1, tmp_b1, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    static uint8_t tmp_b0[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(TMP_B0, tmp_b0, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    temp = ((uint32_t)tmp_b2[1] << 16) | ((uint32_t)tmp_b1[1] << 8) | ((uint32_t)tmp_b0[1]);
    T_raw = getTwosComplement(temp, 24);
    sensor_dps310_calc_temp(&ctx->cal, T_raw);

    /* Read and calculate pressure */

    /* Presure measurment configuration - SPI transfer */
    memset(tx_buf, 0x00, sizeof(tx_buf));
    tx_buf[1] = PM_RATE_8 | PM_PRC_8;
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_write_reg_len(PRS_CFG, tx_buf, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Read PRS_CFG*/
    static uint8_t prs_cfg[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(PRS_CFG, prs_cfg, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Writing the Sensor Operating Mode and Status */
    memset(tx_buf, 0x00, sizeof(tx_buf));
    tx_buf[1] = 0x01;
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_write_reg_len(MEAS_CFG, tx_buf, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Read MEAS_CFG */
    static uint8_t read_mode_P[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(MEAS_CFG, read_mode_P, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    /* Read pressure value - PRS_B2, PRS_B1, PRS_B0 */
    static uint8_t prs_b2[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(PRS_B2, prs_b2, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    static uint8_t prs_b1[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(PRS_B1, prs_b1, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_cs_not_active_set(BOARD_DPS310_CS_PIN);

    static uint8_t prs_b0[2] = { 0, 0 };
    spi_cs_active_set(BOARD_DPS310_CS_PIN);
    DPS_read_reg_len(PRS_B0, prs_b0, 1);
    PROCESS_WAIT_WHILE(mira_spi_transfer_is_in_progress(SPI_ID));
    spi_release(BOARD_DPS310_CS_PIN);

    prs = (prs_b2[1] << 16) | (prs_b1[1] << 8) | (prs_b0[1]);
    P_raw = getTwosComplement(prs, 24);
    float pres_value = sensor_dps310_calc_pres(&ctx->cal, P_raw);

    ctx->val_pressure.value_p = pres_value * 1024;
    ctx->val_pressure.value_q = 1024;

    PROCESS_END();
}