/*
 * Copyright (c) 2024, LumenRadio AB All rights reserved.
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
#include "stdio.h"
#include "network-monitoring.h"

static mira_diag_monitoring_sender_cfg_t mon_config;

void network_monitoring_init(network_monitoring_cfg_t *cfg){
    mon_config.enabled_fields_bitmask = MIRA_DIAG_MONITOR_ALL_FIELDS_ENABLED;
    mon_config.send_interval_s = cfg->update_interval_s;
    if (cfg->enabled == 1 && mon_config.send_interval_s != 0){
        mira_diag_enable_monitoring_sender(&mon_config);
        printf("Network monitoring sender started\n");
    } else {
        mira_diag_disable_monitoring_sender();
    }
}

void network_monitoring_deinit(void){
    mira_diag_disable_monitoring_sender();
}