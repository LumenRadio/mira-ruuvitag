#include <mira.h>
#include <stdio.h>
#include <string.h>

#include "application.h"

#include "app-config.h"
#include "net-status.h"

#define SENSOR_INFO_PORT 7337

PROCESS(main_proc, "Main process");

static uint16_t build_packet(uint8_t *pkt) {
    strcpy((char *)pkt, app_config.name);
    return strlen(app_config.name);
}

PROCESS_THREAD(main_proc, ev, data) {
    static struct etimer tm;
    static struct mira_net_udp_connection_t *conn;
    PROCESS_BEGIN();

    printf("Main process started\n");

    mira_net_encryption_key_set(app_config.net_key);
    mira_net_set_pan_id(app_config.net_panid);
    mira_net_set_rate((mira_net_rate_t)app_config.net_rate);
    mira_net_init();
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
