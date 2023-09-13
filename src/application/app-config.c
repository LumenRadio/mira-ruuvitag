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

#include "app-config.h"

#include <mira.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "board.h"
#include "nfc-if.h"
#include "sensor-lis2dh12.h"

#define APP_CONFIG_EXPOSE_KEY 1

app_config_t app_config;

static app_config_t new_config;
static int new_config_loaded;
static int lost_field = 0;
static int do_restart = 0;
static int acc_config_loaded = 0;

PROCESS(app_config_writer, "Config writer");

static void config_nfc_on_open(
    mira_nfc_ndef_writer_t *writer);
static void config_nfc_on_save(
    uint8_t *file,
    mira_size_t size);
static void config_nfc_on_field_off(
    void);

static const nfcif_handler_t config_nfc_handler = {
    .on_open = config_nfc_on_open,
    .on_save = config_nfc_on_save,
    .on_field_off = config_nfc_on_field_off
};

static void print_config(
    void)
{
    printf("Name: %s\n", app_config.name);
    printf("Pan ID: %8lx\n", app_config.net_panid);
#if APP_CONFIG_EXPOSE_KEY
    printf("Net key:");
    int i;
    for (i = 0; i < 16; i++) {
        printf(" %02x", app_config.net_key[i]);
    }
    printf("\n");
#endif
    printf("Rate: %d\n", (int) app_config.net_rate);
    printf("Update interval: %d\n", (int) app_config.update_interval);
    printf("Move_threshold: %02x\n", app_config.move_threshold);
}

void app_config_init(
    void)
{
    /* Setup nfc */
    if (MIRA_SUCCESS != mira_config_read(&app_config, sizeof(app_config_t))) {
        memset(&app_config, 0xff, sizeof(app_config_t));
    }

    if (!app_config_is_configured()) {
        memset(app_config.name, 0, sizeof(app_config.name));
    }

    print_config();
    new_config_loaded = 0;
    lost_field = 0;

    nfcif_register_handler(&config_nfc_handler);

    process_start(&app_config_writer, NULL);
}

int app_config_is_configured(
    void)
{
    return app_config.net_panid != 0xffffffff && app_config.net_rate != 0xff;
}

static void config_nfc_on_field_off(
    void)
{
    lost_field = 1;
    process_poll(&app_config_writer);
}

static uint8_t *hexstr(
    const uint8_t *data,
    int len)
{
    static uint8_t buf[128];
    uint8_t hexmap[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd',
        'e', 'f'
    };
    int i;
    for (i = 0; i < len; i++) {
        buf[i * 2 + 0] = hexmap[(data[i] >> 4) & 0x0f];
        buf[i * 2 + 1] = hexmap[(data[i] >> 0) & 0x0f];
    }
    return buf;
}

static uint8_t *hexint(
    uint32_t data,
    int len)
{
    uint8_t buf[4];
    buf[0] = (data >> 24) & 0xff;
    buf[1] = (data >> 16) & 0xff;
    buf[2] = (data >> 8) & 0xff;
    buf[3] = (data >> 0) & 0xff;
    return hexstr(buf + 4 - len, len);
}

static int dehexstr(
    void *tgt,
    const uint8_t *src,
    int len)
{
    int i;
    int nibble;
    uint8_t *tgt_i = tgt;
    for (i = 0; i < len; i++) {

        tgt_i[i] = 0;

        for (nibble = 0; nibble < 2; nibble++) {
            char c = src[i * 2 + nibble];
            if (c >= '0' && c <= '9') {
                tgt_i[i] |= (c - '0') << (4 * (1 - nibble));
            } else if (c >= 'a' && c <= 'f') {
                tgt_i[i] |= (c - 'a' + 10) << (4 * (1 - nibble));
            } else if (c >= 'A' && c <= 'F') {
                tgt_i[i] |= (c - 'A' + 10) << (4 * (1 - nibble));
            } else {
                return -1;
            }
        }
    }
    return 0;
}
static int dehexint(
    uint32_t *tgt,
    const uint8_t *src,
    int len)
{
    *tgt = 0;

    uint8_t buf[4];
    memset(buf, 0, 4);
    if (dehexstr(buf + 4 - len, src, len)) {
        return -1;
    }
    *tgt |= ((uint32_t) buf[0]) << 24;
    *tgt |= ((uint32_t) buf[1]) << 16;
    *tgt |= ((uint32_t) buf[2]) << 8;
    *tgt |= ((uint32_t) buf[3]) << 0;

    return 0;
}
static int destrint(
    uint32_t *tgt,
    const uint8_t *src,
    int len)
{
    *tgt = 0;
    uint8_t i, num;
    uint32_t tmp = 0;
    for (i = 0; i < len; i++) {
        num = src[i] - '0';
        if (!(num >= 0 && num <= 9)) {
            return -1;
        }
        tmp += (uint32_t) num * pow(10, (len - 1 - i));
    }
    *tgt = tmp;
    return 0;
}

void config_nfc_on_open(
    mira_nfc_ndef_writer_t *writer)
{

    mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "application/vnd.lumenradio.name", 31,
        NULL, 0,
        (uint8_t *) app_config.name, strlen(app_config.name)
    );
#if APP_CONFIG_EXPOSE_KEY
    mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "application/vnd.lumenradio.net_key", 34,
        NULL, 0,
        hexstr(app_config.net_key, 16), 32
    );
#endif
    mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "application/vnd.lumenradio.net_panid", 36,
        NULL, 0,
        hexint(app_config.net_panid, 4), 8
    );
    char network_rate[10];
    sprintf(network_rate, "%d", app_config.net_rate);
    mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "application/vnd.lumenradio.net_rate", 35,
        NULL, 0,
        (const uint8_t *) network_rate, strlen(network_rate)
    );
    char update_intvl[10];
    sprintf(update_intvl, "%d", app_config.update_interval);
    mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "application/vnd.lumenradio.update_interval", 42,
        NULL, 0,
        (const uint8_t *) update_intvl, strlen(update_intvl)
    );
    char move_thld[10];
    sprintf(move_thld, "%d", app_config.move_threshold);
    mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "application/vnd.lumenradio.move_threshold", 41,
        NULL, 0,
        (const uint8_t *) move_thld, strlen(move_thld)
    );
}

static int cmp_str(
    const char *match,
    const uint8_t *str,
    uint32_t len)
{
    if (strlen(match) != len) {
        return 0;
    }
    return 0 == memcmp(match, str, len);
}

static void config_nfc_on_save(
    uint8_t *file,
    mira_size_t size)
{
    mira_nfc_ndef_iter_t iter;
    mira_nfc_ndef_record_t rec;

    if (new_config_loaded) {
        /* Already loaded, waiting for reset */
        return;
    }

    memcpy(&new_config, &app_config, sizeof(app_config_t));

    if (size == 0) {
        return;
    }
    for (mira_nfc_ndef_iter_start(&iter, &rec, file, size);
         mira_nfc_ndef_iter_valid(&iter);
         mira_nfc_ndef_iter_next(&iter, &rec)) {
        if (rec.type_name_format != MIRA_NFC_NDEF_TNF_MIME_TYPE) {
            continue;
        }

        if (cmp_str(
            "application/vnd.lumenradio.name",
            rec.type,
            rec.type_length)) {
            int len = sizeof(new_config.name) - 1;
            if (len > rec.payload_length) {
                len = rec.payload_length;
            }

            memset(new_config.name, 0, sizeof(new_config.name));
            memcpy(new_config.name, rec.payload, len);
        }
        if (cmp_str(
            "application/vnd.lumenradio.net_key",
            rec.type,
            rec.type_length)) {
            if (rec.payload_length != 32
                || dehexstr(new_config.net_key, rec.payload, 16)) {
                printf("Invalid net_key\n");
                return;
            }
            if (new_config.net_key == app_config.net_key) {
                do_restart = 0;
            } else {
                do_restart = 1;
            }
        }
        if (cmp_str(
            "application/vnd.lumenradio.net_panid",
            rec.type,
            rec.type_length)) {
            uint32_t tmp;
            if (rec.payload_length != 8 || dehexint(&tmp, rec.payload, 4)) {
                printf("Invalid net_panid \n");
                return;
            }
            new_config.net_panid = tmp;
            if (new_config.net_panid == app_config.net_panid) {
                do_restart = 0;
            } else {
                do_restart = 1;
            }
        }
        if (cmp_str(
            "application/vnd.lumenradio.net_rate",
            rec.type,
            rec.type_length)) {
            uint32_t tmp;
            if ((rec.payload_length < 1 && rec.payload_length > 2)
                || destrint(&tmp, rec.payload, rec.payload_length)) {
                printf("Invalid net_rate\n");
                return;
            }
            if (tmp < 0 || tmp > 10) {
                tmp = 255;
            }
            new_config.net_rate = tmp;
            if (new_config.net_rate == app_config.net_rate) {
                do_restart = 0;
            } else {
                do_restart = 1;
            }
        }
        if (cmp_str(
            "application/vnd.lumenradio.update_interval",
            rec.type,
            rec.type_length)) {
            uint32_t tmp;
            if ((rec.payload_length < 1 && rec.payload_length > 5)
                || destrint(&tmp, rec.payload, rec.payload_length)) {
                printf("Invalid update_interval\n");
                return;
            }
            if (tmp > 65535 || tmp < 0) {
                tmp = 65535;
            }
            new_config.update_interval = tmp;
        }
        if (cmp_str(
            "application/vnd.lumenradio.move_threshold",
            rec.type,
            rec.type_length)) {
            uint32_t tmp;
            if ((rec.payload_length < 1 && rec.payload_length > 3)
                || destrint(&tmp, rec.payload, rec.payload_length)) {
                printf("Invalid move threshold\n");
                return;
            }
            if (tmp > 127 || tmp < 0) {
                tmp = 127;
            }
            new_config.move_threshold = tmp;
            if (new_config.move_threshold != app_config.move_threshold) {
                acc_config_loaded = 1;
            }
        }
    }

    new_config_loaded = 1;
}

PROCESS_THREAD(app_config_writer, ev, data)
{
    mira_status_t status;
    PROCESS_BEGIN();
    while (1) {
        PROCESS_YIELD_UNTIL(lost_field);
        lost_field = 0;

        if (new_config_loaded) {
            printf(
                "Writing config, Pan ID: %08lx rate: %02x\n",
                new_config.net_panid,
                new_config.net_rate);

            new_config_loaded = 0;
            status = mira_config_write(&new_config, sizeof(app_config_t));
            printf("New config loaded status: %d\n", status);
            PROCESS_WAIT_WHILE(mira_config_is_working());

            printf("Done\n");
            if (do_restart) {
                printf("Restarting\n");
                mira_sys_reset();
            }
            do_restart = 0;
            /* If we don't do a restart, copy new config to app_config */
            memcpy(&app_config, &new_config, sizeof(app_config_t));

            /* Restarting process after loading new accelerometer config */
            if (acc_config_loaded) {
                sensor_lis2dh12_reinit_sensor();
            }
            acc_config_loaded = 0;
            print_config();
        }
    }
    PROCESS_END();
}
