#include "app-config.h"

#include <mira.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "nfc-if.h"

#define APP_CONFIG_EXPOSE_KEY 1

app_config_t app_config;
int new_config_loaded;

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
    for (i = 0; i < 16; i++)
    {
        printf(" %02x", app_config.net_key[i]);
    }
    printf("\n");
#endif
    printf("Rate: %d\n", (int) app_config.net_rate);
    printf("Update interval: %d\n", (int) app_config.update_interval);
}

void app_config_init(
    void)
{
    /* Setup nfc */
    if (MIRA_SUCCESS != mira_config_read(&app_config, 0, sizeof(app_config_t)))
    {
        memset(&app_config, 0xff, sizeof(app_config_t));
    }

    if (!app_config_is_configured())
    {
        memset(app_config.name, 0, sizeof(app_config.name));
    }

    print_config();
    new_config_loaded = 0;

    nfcif_register_handler(&config_nfc_handler);
}

int app_config_is_configured(
    void)
{
    return app_config.net_panid != 0xffffffff && app_config.net_rate != 0xff;
}

static void config_nfc_on_field_off(
    void)
{
    if(new_config_loaded) {
        mira_sys_reset();
    }
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
    for (i = 0; i < len; i++)
    {
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
    for (i = 0; i < len; i++)
    {

        tgt_i[i] = 0;

        for (nibble = 0; nibble < 2; nibble++)
        {
            char c = src[i * 2 + nibble];
            if (c >= '0' && c <= '9')
            {
                tgt_i[i] |= (c - '0') << (4 * (1 - nibble));
            }
            else if (c >= 'a' && c <= 'f')
            {
                tgt_i[i] |= (c - 'a' + 10) << (4 * (1 - nibble));
            }
            else if (c >= 'A' && c <= 'F')
            {
                tgt_i[i] |= (c - 'A' + 10) << (4 * (1 - nibble));
            }
            else
            {
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
    if (dehexstr(buf + 4 - len, src, len))
    {
        return -1;
    }
    *tgt |= ((uint32_t) buf[0]) << 24;
    *tgt |= ((uint32_t) buf[1]) << 16;
    *tgt |= ((uint32_t) buf[2]) << 8;
    *tgt |= ((uint32_t) buf[3]) << 0;

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
    mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "application/vnd.lumenradio.net_rate", 35,
        NULL, 0,
        hexint(app_config.net_rate, 1), 2
        );
    mira_nfc_ndef_write_copy(writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "application/vnd.lumenradio.update_interval", 42,
        NULL, 0,
        hexint(app_config.update_interval, 2), 4
        );
}

static int cmp_str(
    const char *match,
    const uint8_t *str,
    uint32_t len)
{
    if (strlen(match) != len)
    {
        return 0;
    }
    return 0 == memcmp(match, str, len);
}

static void config_nfc_on_save(
    uint8_t *file,
    mira_size_t size)
{
    app_config_t new_config;
    mira_nfc_ndef_iter_t iter;
    mira_nfc_ndef_record_t rec;

    memcpy(&new_config, &app_config, sizeof(app_config_t));

    if (size == 0)
    {
        return;
    }
    for (mira_nfc_ndef_iter_start(&iter, &rec, file, size);
        mira_nfc_ndef_iter_valid(&iter);
        mira_nfc_ndef_iter_next(&iter, &rec))
    {
        if (rec.type_name_format != MIRA_NFC_NDEF_TNF_MIME_TYPE)
        {
            continue;
        }

        if (cmp_str(
            "application/vnd.lumenradio.name",
            rec.type,
            rec.type_length))
        {
            int len = sizeof(new_config.name) - 1;
            if (len > rec.payload_length)
            {
                len = rec.payload_length;
            }

            memset(new_config.name, 0, sizeof(new_config.name));
            memcpy(new_config.name, rec.payload, len);
        }
        if (cmp_str(
            "application/vnd.lumenradio.net_key",
            rec.type,
            rec.type_length))
        {
            if (rec.payload_length != 32
                || dehexstr(new_config.net_key, rec.payload, 16))
            {
                printf("Invalid net_key\n");
                return;
            }
        }
        if (cmp_str(
            "application/vnd.lumenradio.net_panid",
            rec.type,
            rec.type_length))
        {
            uint32_t tmp;
            if (rec.payload_length != 8 || dehexint(&tmp, rec.payload, 4))
            {
                printf("Invalid net_panid \n");
                return;
            }
            new_config.net_panid = tmp;
        }
        if (cmp_str(
            "application/vnd.lumenradio.net_rate",
            rec.type,
            rec.type_length))
        {
            uint32_t tmp;
            if (rec.payload_length != 2 || dehexint(&tmp, rec.payload, 1))
            {
                printf("Invalid net_rate\n");
                return;
            }
            new_config.net_rate = tmp;
        }
        if (cmp_str(
            "application/vnd.lumenradio.update_interval",
            rec.type,
            rec.type_length))
        {
            uint32_t tmp;
            if (rec.payload_length != 4 || dehexint(&tmp, rec.payload, 2))
            {
                printf("Invalid update_interval\n");
                return;
            }
            new_config.update_interval = tmp;
        }
    }

    mira_config_erase();
    mira_config_write(&new_config, 0, sizeof(app_config_t));
    new_config_loaded = 1;
}
