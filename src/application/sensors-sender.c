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
#include <stdio.h>
#include <string.h>

#define SENSOR_PORT 7338
/* To avoid fragmentation, prevent packet size from being larger than 160 bytes.
 * Therefore, only (160-16-40-4)/9 = 11 sensors values may be sent */
#define MAX_SENSOR_VALUES 11

void
sensors_sender_init(sensors_sender_context_t* ctx)
{
    /*
     * Connect to have a reference for MIRA. But we don't care about the
     * responses, and only want to use mira_net_udp_send_to() later.
     */
    ctx->conn = mira_net_udp_connect(NULL, 0, NULL, NULL);
    ctx->seq_no = 0;
}

static int
sensors_sender_add_value(uint8_t* buf, const sensor_value_t* value)
{
    /*
     * Each sensor value is:
     * - 1 byte type
     * - 4 byte value, signed int, network byte order
     * - 4 byte fix point, unsigned int, network byte order */
    size_t buf_idx = 0;
    buf[buf_idx++] = value->type;

    buf[buf_idx++] = (value->value_p >> 24) & 0xff;
    buf[buf_idx++] = (value->value_p >> 16) & 0xff;
    buf[buf_idx++] = (value->value_p >> 8) & 0xff;
    buf[buf_idx++] = (value->value_p >> 0) & 0xff;

    buf[buf_idx++] = (value->value_q >> 24) & 0xff;
    buf[buf_idx++] = (value->value_q >> 16) & 0xff;
    buf[buf_idx++] = (value->value_q >> 8) & 0xff;
    buf[buf_idx++] = (value->value_q >> 0) & 0xff;

    return buf_idx;
}

static int
sensors_sender_add_header(uint8_t* buf, uint32_t seq_no)
{
    mira_net_address_t parent_address;
    char parent_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    memset(parent_address_str, 0, MIRA_NET_MAX_ADDRESS_STR_LEN);

    if (mira_net_get_parent_address(&parent_address) != MIRA_SUCCESS) {
        printf("No parent address: %d\n", mira_net_get_state());
    } else {
        mira_net_toolkit_format_address(parent_address_str, &parent_address);
    }

    /*
     * Format is:
     * - 16 bytes name
     * - 40 bytes parent address
     * - 4 byte sequence number, network byte order
     */
    size_t payload_len = 0;
    memcpy(&buf[payload_len], app_config.name, 16);
    payload_len += 16;
    memcpy(&buf[payload_len], &parent_address_str, MIRA_NET_MAX_ADDRESS_STR_LEN);
    payload_len += MIRA_NET_MAX_ADDRESS_STR_LEN;

    buf[payload_len++] = (seq_no >> 24) & 0xff;
    buf[payload_len++] = (seq_no >> 16) & 0xff;
    buf[payload_len++] = (seq_no >> 8) & 0xff;
    buf[payload_len++] = (seq_no >> 0) & 0xff;

    return payload_len;
}

void
sensors_sender_send(sensors_sender_context_t* ctx, const sensor_value_t** values, int num_values)
{
    mira_net_address_t root_address;
    char parent_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    memset(parent_address_str, 0, MIRA_NET_MAX_ADDRESS_STR_LEN);
    if (mira_net_get_root_address(&root_address) != MIRA_SUCCESS) {
        printf("No root address: %d\n", mira_net_get_state());
        return;
    }

    uint8_t payload[160];
    size_t payload_len = 0;
    payload_len += sensors_sender_add_header(&payload[payload_len], ctx->seq_no);

    int nrof_added_values = 0;
    for (int i = 0; values[i] != NULL; i++) {
        if (nrof_added_values >= MAX_SENSOR_VALUES) {
            break;
        }
        if (values[i]->type == SENSOR_VALUE_TYPE_NONE) {
            // Skip uniniatlized sensors
            continue;
        }

        payload_len += sensors_sender_add_value(&payload[payload_len], values[i]);
        nrof_added_values += 1;
    }

    mira_net_udp_send_to(ctx->conn, &root_address, SENSOR_PORT, payload, payload_len);
    ctx->seq_no++;
}
