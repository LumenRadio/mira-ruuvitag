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
#include <stdio.h>
#include <string.h>

#include "application.h"

#include "app-config.h"
#include "net-status.h"
#include "sensor-bme280.h"

#define SENSOR_INFO_PORT 7337

PROCESS(main_proc, "Main process");


static uint16_t build_packet(uint8_t *pkt) {
    strcpy((char *)pkt, app_config.name);
    return strlen(app_config.name);
}

PROCESS_THREAD(main_proc, ev, data) {
    static struct etimer tm;
    static struct mira_net_udp_connection_t *conn;
    mira_net_config_t netconf;
    PROCESS_BEGIN();

    printf("Main process started\n");

    sensor_bme280_init();

    netconf.pan_id = app_config.net_panid;
    memcpy(netconf.key, app_config.net_key, 16);
    netconf.mode = MIRA_NET_MODE_MESH;
    netconf.rate = app_config.net_rate;
    netconf.antenna = MIRA_NET_ANTENNA_ONBOARD;

    mira_net_init(&netconf);

    net_status_init();

    conn = mira_net_udp_connect(NULL, 0, NULL, NULL);

    while(1) {
        mira_net_address_t root_addr;
        uint8_t pkt_buf[256];
        uint16_t pkt_len;

        if(MIRA_SUCCESS == mira_net_get_root_address(&root_addr)) {
            pkt_len = build_packet(pkt_buf);
            if(pkt_len > 0) {
                mira_net_udp_send_to(
                    conn,
                    &root_addr,
                    SENSOR_INFO_PORT,
                    pkt_buf,
                    pkt_len);
            }
        } else {
            /* No root found */
            printf("No root found\n");
        }

        etimer_set(&tm, CLOCK_SECOND * app_config.update_interval);
        PROCESS_YIELD_UNTIL(etimer_expired(&tm));
    }

    mira_net_udp_close(conn);

    PROCESS_END();
}
