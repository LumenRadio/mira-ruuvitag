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

#include "sensors-sender.h"
#include "app-config.h"
#include <mira.h>
#include <string.h>
#include <stdio.h>

#define SENSOR_PORT 7338
/* To avoid fragmentation, prevent packet size from being larger than 160 bytes. Therefore, only
 * (160-16-40-4)/9 = 11 sensors values may be sent */
#define MAX_SENSOR_VALUES 11

void sensors_sender_init(
    sensors_sender_context_t *ctx)
{
    /*
     * Connect to have a reference for MIRA. But we don't care about the
     * responses, and only want to use mira_net_udp_send_to() later.
     */
    ctx->conn = mira_net_udp_connect(NULL, 0, NULL, NULL);
    ctx->seq_no = 0;
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
    mira_net_address_t parent_address;
    char parent_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    memset(parent_address_str, 0, MIRA_NET_MAX_ADDRESS_STR_LEN);

    if (mira_net_get_root_address(&root_address) != MIRA_SUCCESS) {
        printf("No root address: %d\n", mira_net_get_state() );
        return;
    }

    if (mira_net_get_parent_address(&parent_address) != MIRA_SUCCESS) {
        printf("No parent address: %d\n", mira_net_get_state());
    } else {
        mira_net_toolkit_format_address(parent_address_str, &parent_address);
    }

    /*
     * Format is:
     * - 16 bytes name
     * - 40 bytes parent address
     * - n*9 bytes sensor values
     *
     * Each sensor value is:
     * - 1 byte type
     * - 4 byte value, signed int , MSB first
     * - 4 byte fix point, unsigned int, MSB first
     */

    payload_len = 0;

    memcpy(&payload[payload_len], app_config.name, 16);
    payload_len += 16;
    memcpy(&payload[payload_len], &parent_address_str,
        MIRA_NET_MAX_ADDRESS_STR_LEN);
    payload_len += MIRA_NET_MAX_ADDRESS_STR_LEN;

    int nrof_added_values = 0;
    for (i = 0; values[i] != NULL; i++) {
        if (nrof_added_values >= MAX_SENSOR_VALUES) {
            break;
        }
        if (values[i]->type == SENSOR_VALUE_TYPE_NONE) {
            // Skip uniniatlized sensors
            continue;
        }

        const sensor_value_t *val = values[i];
        payload[payload_len + 0] = values[i]->type;
        if (values[i]->type == SENSOR_VALUE_TYPE_SEQ_NO) {
            const int seq = ctx->seq_no;
            payload[payload_len + 1] = (seq >> 24) & 0xff;
            payload[payload_len + 2] = (seq >> 16) & 0xff;
            payload[payload_len + 3] = (seq >> 8) & 0xff;
            payload[payload_len + 4] = (seq >> 0) & 0xff;
            ctx->seq_no++;
        } else {
            payload[payload_len + 1] = (val->value_p >> 24) & 0xff;
            payload[payload_len + 2] = (val->value_p >> 16) & 0xff;
            payload[payload_len + 3] = (val->value_p >> 8) & 0xff;
            payload[payload_len + 4] = (val->value_p >> 0) & 0xff;
        }

        payload[payload_len + 5] = (val->value_q >> 24) & 0xff;
        payload[payload_len + 6] = (val->value_q >> 16) & 0xff;
        payload[payload_len + 7] = (val->value_q >> 8) & 0xff;
        payload[payload_len + 8] = (val->value_q >> 0) & 0xff;
        payload_len += 9;
        nrof_added_values += 1;
    }

    mira_net_udp_send_to(
        ctx->conn,
        &root_address,
        SENSOR_PORT,
        payload,
        payload_len);
}
