#include "sensors-sender.h"
#include "app-config.h"
#include <mira.h>
#include <string.h>

#define SENSOR_PORT 7337

void sensors_sender_init(
    sensors_sender_context_t *ctx)
{
    /*
     * Connect to have a reference for MIRA. But we don't care about the
     * responses, and only want to use mira_net_udp_send_to() later.
     */
    ctx->conn = mira_net_udp_connect(NULL, 0, NULL, NULL);
}

void sensors_sender_send(
    sensors_sender_context_t *ctx,
    const sensor_value_t **values,
    int num_values)
{
    uint8_t payload[160];
    int payload_len;
    int i;

    mira_net_address_t root_address;

    /*
     * Format is:
     * - 32 bytes name
     * - n*24 bytes sensor values
     *
     * Each sensor value is:
     * - 15 bytes name
     * - 1 byte unit
     * - 4 byte value, signed int , MSB first
     * - 4 byte fix point, unsigned int, MSB first
     *
     * Total packet size may not be bigger than 160 bytes. Therefore, only
     * (160-32)/24 = 5 sensors values may be sent
     */
    if(num_values > 5) {
        num_values = 5;
    }

    payload_len = 0;

    memcpy(&payload[payload_len], app_config.name, 32);
    payload_len += 32;

    for(i=0; i<num_values; i++) {
        const sensor_value_t *val = values[i];
        memcpy(&payload[payload_len+0], val->name, 15);
        payload[payload_len+15] = values[i]->unit;

        payload[payload_len+16] = (val->value_p >> 24) & 0xff;
        payload[payload_len+17] = (val->value_p >> 16) & 0xff;
        payload[payload_len+18] = (val->value_p >>  8) & 0xff;
        payload[payload_len+19] = (val->value_p >>  0) & 0xff;

        payload[payload_len+20] = (val->value_q >> 24) & 0xff;
        payload[payload_len+21] = (val->value_q >> 16) & 0xff;
        payload[payload_len+22] = (val->value_q >>  8) & 0xff;
        payload[payload_len+23] = (val->value_q >>  0) & 0xff;
        payload_len += 24;
    }

    if(mira_net_get_root_address(&root_address) != MIRA_SUCCESS) {
        return;
    }

    mira_net_udp_send_to(
        ctx->conn,
        &root_address,
        SENSOR_PORT,
        payload,
        payload_len);
}
