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
#include "nfc-if.h"

static nfcif_handler_t nfc_handlers[16];
static int nfc_n_handlers;

static uint8_t nfc_file_buffer[1024];


static void nfc_field_on(
    void *storage)
{
    int i;
    printf("Field on\n");

    for(i=0; i<nfc_n_handlers; i++) {
        if(nfc_handlers[i].on_field_on) {
            nfc_handlers[i].on_field_on();
        }
    }
}

static void nfc_field_off(
    void *storage)
{
    int i;
    printf("Field off\n");

    for(i=0; i<nfc_n_handlers; i++) {
        if(nfc_handlers[i].on_field_off) {
            nfc_handlers[i].on_field_off();
        }
    }
}

static uint8_t *nfc_file_open(
    mira_nfc_file_id_t file_id,
    mira_size_t *size,
    void *storage)
{
    int i;
    mira_nfc_ndef_writer_t writer;

    const char *version_string = "RuuviMira";

    if (file_id == MIRA_NFC_NDEF_FILE_ID)
    {
        printf("NDEF file open\n");

        mira_nfc_ndef_write_start(
            &writer,
            nfc_file_buffer,
            sizeof(nfc_file_buffer));

        /* First field should be of identifiable header */
        mira_nfc_ndef_write_copy(&writer, MIRA_NFC_NDEF_TNF_MIME_TYPE,
            (const uint8_t *) "application/vnd.lumenradio.mira2", 32,
            NULL, 0,
            (uint8_t *)version_string, strlen(version_string)
            );

        /* Add fields by handlers */
        for(i=0; i<nfc_n_handlers; i++) {
            if(nfc_handlers[i].on_open) {
                nfc_handlers[i].on_open(&writer);
            }
        }

        mira_nfc_ndef_write_end(&writer, size);
        return nfc_file_buffer;
    }
    printf("Trying to open unknown file 0x%04x\n", file_id);
    *size = 0;
    return NULL;
}

static mira_bool_t nfc_file_save(
    mira_nfc_file_id_t file_id,
    mira_size_t size,
    void *storage)
{
    int i;
    if (file_id == MIRA_NFC_NDEF_FILE_ID)
    {
        printf("NDEF file save of size %lu\n", size);

        /* Give all handlers a chanse to process */
        for(i=0; i<nfc_n_handlers; i++) {
            if(nfc_handlers[i].on_save) {
                nfc_handlers[i].on_save(nfc_file_buffer, size);
            }
        }

        return MIRA_TRUE;
    }
    printf("Trying to save %lu bytes to unknown file %04x\n", size, file_id);
    return MIRA_FALSE;
}

static mira_nfc_config_t nfc_conf = {
    .callback_field_on = nfc_field_on,
    .callback_field_off = nfc_field_off,
    .callback_file_open = nfc_file_open,
    .callback_file_save = nfc_file_save,
    .storage = NULL,
    .max_file_size = sizeof(nfc_file_buffer),
    .proprietary_file_count = 0
};

void nfcif_init(
    void)
{
    nfc_n_handlers = 0;
    mira_nfc_init(&nfc_conf);
}

int nfcif_register_handler(
    const nfcif_handler_t *handler)
{
    nfc_handlers[nfc_n_handlers] = *handler;
    nfc_n_handlers++;
    return 0;
}
