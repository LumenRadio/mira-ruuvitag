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
    mira_net_address_t parent_address;
    char parent_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    memset(parent_address_str,0, MIRA_NET_MAX_ADDRESS_STR_LEN);

    if(mira_net_get_parent_address(&parent_address) != MIRA_SUCCESS) {
        printf("No parent address.\n");
    } else {
        mira_net_toolkit_format_address(parent_address_str, &parent_address);
    }

    /*
     * Format is:
     * - 32 bytes name
     * - 40 bytes parent address
     * - n*9 bytes sensor values
     *
     * Each sensor value is:
     * - 1 byte type name
     * - 4 byte value, signed int , MSB first
     * - 4 byte fix point, unsigned int, MSB first
     *
     * Total packet size may not be bigger than 160 bytes. Therefore, only
     * (160-32-40)/9 = 9 sensors values may be sent
     */
    if(num_values > 9) {
        num_values = 9;
    }

    payload_len = 0;

    memcpy(&payload[payload_len], app_config.name, 32);
    payload_len += 32;
    memcpy(&payload[payload_len], &parent_address_str, MIRA_NET_MAX_ADDRESS_STR_LEN);
    payload_len += MIRA_NET_MAX_ADDRESS_STR_LEN;


    for(i=0; i<num_values; i++) {
        const sensor_value_t *val = values[i];
        //memcpy(&payload[payload_len+0], val->name, 15); old
        payload[payload_len+0] = values[i]->type;

        payload[payload_len+1] = (val->value_p >> 24) & 0xff;
        payload[payload_len+2] = (val->value_p >> 16) & 0xff;
        payload[payload_len+3] = (val->value_p >>  8) & 0xff;
        payload[payload_len+4] = (val->value_p >>  0) & 0xff;

        payload[payload_len+5] = (val->value_q >> 24) & 0xff;
        payload[payload_len+6] = (val->value_q >> 16) & 0xff;
        payload[payload_len+7] = (val->value_q >>  8) & 0xff;
        payload[payload_len+8] = (val->value_q >>  0) & 0xff;
        payload_len += 9;
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


/* ATT ÄNDRA:
* minska 15 bytes name till enbart 1 byte lista.
* lägg till etx som mätvärde
* lägg till drift som mätvärrde
* lägg till parent efter name
*/
