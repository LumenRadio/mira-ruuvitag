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

#ifndef SENSOR_DPS310_H
#define SENSOR_DPS310_H

#include "sensor-value.h"
#include <mira.h>

typedef struct
{
    int32_t m_c0Half;
    int32_t m_c1;
    int32_t m_c00;
    int32_t m_c10;
    int32_t m_c01;
    int32_t m_c11;
    int32_t m_c20;
    int32_t m_c21;
    int32_t m_c30;
} sensor_dps310_calib_t;

typedef struct
{
    sensor_dps310_calib_t cal;
    sensor_value_t val_pressure;
    mira_bool_t sensor_available;
} sensor_dps310_ctx_t;

/**
 * Call and wait to finish for sensor to start up
 */
PROCESS_NAME(sensor_dps310_init);

/**
 * Call and wait to finish for sensor to sample data
 */
PROCESS_NAME(sensor_dps310_sample);

#endif
