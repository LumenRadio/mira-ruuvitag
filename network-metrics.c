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
#include <string.h>
#include <stdio.h>

#include "network-metrics.h"

#if NETWORK_METRICS_DRIFT_ENABLED
extern int32_t slotop_timesync_drift_ppm;
#endif

PROCESS(network_metrics_init, "Network metrics: startup");
PROCESS(network_metrics_sample, "network_metrics: sample network metrics");

PROCESS_THREAD(
    network_metrics_init,
    ev,
    data)
{
    static network_metrics_ctx_t *ctx;

    PROCESS_BEGIN();

    ctx = data;

    memset(ctx, 0, sizeof(network_metrics_ctx_t));

    printf("Network metrics started\n");

    PROCESS_PAUSE();

    ctx->etx.type = SENSOR_VALUE_TYPE_ETX;
#if NETWORK_METRICS_DRIFT_ENABLED
    ctx->clock_drift.type = SENSOR_VALUE_TYPE_CLOCK_DRIFT;
#endif

    PROCESS_END();
}


PROCESS_THREAD(
    network_metrics_sample,
    ev,
    data)
{
    static network_metrics_ctx_t *ctx;

    PROCESS_BEGIN();

    ctx = data;

    PROCESS_PAUSE();

    ctx->etx.value_p = mira_net_get_parent_link_metric();
    ctx->etx.value_q = 128;

#if NETWORK_METRICS_DRIFT_ENABLED
    ctx->clock_drift.value_p = slotop_timesync_drift_ppm;
    ctx->clock_drift.value_q = 256000000;
#endif

    PROCESS_END();
}
