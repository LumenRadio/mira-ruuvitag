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

#include "sensor-value.h"

const char *sensor_value_unit_name[] = {
    [SENSOR_VALUE_TYPE_NONE] = "none",
    [SENSOR_VALUE_TYPE_TEMPERATURE] = "deg C",
    [SENSOR_VALUE_TYPE_PRESSURE] = "Pa",
    [SENSOR_VALUE_TYPE_HUMIDITY] = "%",
    [SENSOR_VALUE_TYPE_BATTERY] = "V",
    [SENSOR_VALUE_TYPE_ETX] = "",
    [SENSOR_VALUE_TYPE_CLOCK_DRIFT] = ""
};

const char *sensor_value_type_name[] = {
    [SENSOR_VALUE_TYPE_NONE] = "none",
    [SENSOR_VALUE_TYPE_TEMPERATURE] = "temperature",
    [SENSOR_VALUE_TYPE_PRESSURE] = "pressure",
    [SENSOR_VALUE_TYPE_HUMIDITY] = "humidity",
    [SENSOR_VALUE_TYPE_BATTERY] = "battery",
    [SENSOR_VALUE_TYPE_ETX] = "etx",
    [SENSOR_VALUE_TYPE_CLOCK_DRIFT] = "clock_drift"
};
