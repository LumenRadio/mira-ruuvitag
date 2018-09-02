#ifndef NFC_IF_H_
#define NFC_IF_H_

#include <mira.h>

typedef struct
{
    void (*on_open)(
        mira_nfc_ndef_writer_t *writer);
    void (*on_save)(
        uint8_t *file,
        mira_size_t size);
    void (*on_field_on)(
        void);
    void (*on_field_off)(
        void);
} nfcif_handler_t;

void nfcif_init(
    void);

int nfcif_register_handler(
    const nfcif_handler_t *handler);

#endif
