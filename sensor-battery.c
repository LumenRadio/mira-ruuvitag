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

#include <mira.h>
#include "sensor-battery.h"

#include <string.h>

PROCESS(sensor_battery_init, "Sensor: Battery voltage startup");
PROCESS(sensor_battery_sample, "Sensor: Battery voltage reader");

PROCESS_THREAD(
    sensor_battery_init,
    ev,
    data)
{
    static sensor_battery_ctx_t *ctx;

    PROCESS_BEGIN();

    ctx = data;

    memset(ctx, 0, sizeof(sensor_battery_ctx_t));

    PROCESS_PAUSE();

    mira_adc_init(&ctx->adc);
    mira_adc_set_source_single(&ctx->adc, MIRA_ADC_PIN_VDD);
    mira_adc_set_reference(&ctx->adc, MIRA_ADC_REF_INT_3_6V);

    strcpy(ctx->val_battery.name, "battery");


    PROCESS_END();
}


PROCESS_THREAD(
    sensor_battery_sample,
    ev,
    data)
{
    static sensor_battery_ctx_t *ctx;

    mira_adc_value_t value;

    PROCESS_BEGIN();

    ctx = data;

    PROCESS_PAUSE();

    ctx->val_battery.unit = SENSOR_VALUE_UNIT_NONE;
    ctx->val_battery.value_p = 0;
    ctx->val_battery.value_q = 0;

    mira_adc_measurement_start(&ctx->adc);
    PROCESS_WAIT_WHILE(mira_adc_measurement_in_progress(&ctx->adc));
    if(mira_adc_measurement_finish(&ctx->adc, &value) == MIRA_SUCCESS) {
        ctx->val_battery.unit = SENSOR_VALUE_UNIT_VOLT;
        ctx->val_battery.value_p = value;
        ctx->val_battery.value_q = 327680/36;
    }


    PROCESS_END();
}
