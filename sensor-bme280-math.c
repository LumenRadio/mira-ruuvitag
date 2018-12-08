#include "sensor-bme280-math.h"


void sensor_bme280_math_populate_calib(
    sensor_bme280_calib_t *cal,
    uint8_t *regs88,
    uint8_t *regse1)
{
    cal->dig_T1 = (uint16_t) regs88[0] | ((uint16_t) regs88[1]) << 8;
    cal->dig_T2 = (int16_t) regs88[2] | ((uint16_t) regs88[3]) << 8;
    cal->dig_T3 = (int16_t) regs88[4] | ((uint16_t) regs88[5]) << 8;
    cal->dig_P1 = (uint16_t) regs88[6] | ((uint16_t) regs88[7]) << 8;
    cal->dig_P2 = (int16_t) regs88[8] | ((int16_t) regs88[9]) << 8;
    cal->dig_P3 = (int16_t) regs88[10] | ((int16_t) regs88[11]) << 8;
    cal->dig_P4 = (int16_t) regs88[12] | ((int16_t) regs88[13]) << 8;
    cal->dig_P5 = (int16_t) regs88[14] | ((int16_t) regs88[15]) << 8;
    cal->dig_P6 = (int16_t) regs88[16] | ((int16_t) regs88[17]) << 8;
    cal->dig_P7 = (int16_t) regs88[18] | ((int16_t) regs88[19]) << 8;
    cal->dig_P8 = (int16_t) regs88[20] | ((int16_t) regs88[21]) << 8;
    cal->dig_P9 = (int16_t) regs88[22] | ((int16_t) regs88[23]) << 8;
    cal->dig_H1 = (uint8_t) regs88[25];
    cal->dig_H2 = (int16_t) regse1[0] | ((int16_t) regse1[1]) << 8;
    cal->dig_H3 = (uint8_t) regse1[2];
    cal->dig_H4 = ((uint16_t) regse1[4] & 0x0f)
        | ((int16_t) (int8_t) regse1[3]) << 4;
    cal->dig_H5 = ((uint16_t) (regse1[4] & 0xf0) >> 4)
        | ((int16_t) (int8_t) regse1[5]) << 4;
    cal->dig_H6 = (int8_t) regse1[6];
}

int32_t sensor_bme280_math_calc_tfine(
    sensor_bme280_calib_t *cal,
    int32_t adc_T)
{
    int32_t var1, var2;
    var1 = ((((adc_T >> 3) - ((int32_t)cal->dig_T1<<1))) * ((int32_t)cal->dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)cal->dig_T1)) * ((adc_T>>4) - ((int32_t)cal->dig_T1))) >> 12) * ((int32_t)cal->dig_T3)) >> 14;
    return var1 + var2;
}

int32_t sensor_bme280_math_calc_t(
    int32_t t_fine)
{
    return t_fine * 5 + 128;
}

uint32_t sensor_bme280_math_calc_p(
    sensor_bme280_calib_t *cal,
    int32_t adc_P,
    int32_t t_fine)
{
    int64_t var1, var2, p;
    var1 = ((int64_t) t_fine) - 128000;
    var2 = var1 * var1 * (int64_t) cal->dig_P6;
    var2 = var2 + ((var1 * (int64_t) cal->dig_P5) << 17);
    var2 = var2 + (((int64_t) cal->dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t) cal->dig_P3) >> 8) + ((var1 * (int64_t) cal->dig_P2) << 12);
    var1 = (((((int64_t) 1) << 47) + var1)) * ((int64_t) cal->dig_P1) >> 33;
    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t) cal->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t) cal->dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t) cal->dig_P7) << 4);
    return (uint32_t) p;
}

uint32_t sensor_bme280_math_calc_h(
    sensor_bme280_calib_t *cal,
    int32_t adc_H,
    int32_t t_fine)
{
    int32_t v_x1_u32r;
    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)cal->dig_H4) << 20) - (((int32_t)cal->dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)cal->dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)cal->dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)cal->dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)cal->dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint32_t) (v_x1_u32r >> 12);
}
