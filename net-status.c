#include <mira.h>
#include <string.h>
#include "net-status.h"
#include "nfc-if.h"

static void net_status_nfc_on_open(
    mira_nfc_ndef_writer_t *writer)
{
    const char *net_state[4] = {
        "not associated",
        "is coordinator",
        "associated",
        "joined"
    };

    mira_net_state_t state;

    mira_net_address_t address;
    char addr_str[MIRA_NET_MAX_ADDRESS_STR_LEN];

    state = mira_net_get_state();
    mira_nfc_ndef_write_copy(writer,
        MIRA_NFC_NDEF_TNF_MIME_TYPE,
        (const uint8_t *) "text/plain", 10,
        (const uint8_t *) "net.state", 9,
        (const uint8_t *) net_state[state], strlen(net_state[state])
            );

    if(mira_net_get_address(&address) == MIRA_SUCCESS) {
        mira_net_toolkit_format_address(addr_str, &address);
        mira_nfc_ndef_write_copy(writer,
            MIRA_NFC_NDEF_TNF_MIME_TYPE,
            (const uint8_t *) "text/plain", 10,
            (const uint8_t *) "net.address", 11,
            (const uint8_t *) addr_str, strlen(addr_str)
                );
    }

    if(mira_net_get_parent_address(&address) == MIRA_SUCCESS) {
        mira_net_toolkit_format_address(addr_str, &address);
        mira_nfc_ndef_write_copy(writer,
            MIRA_NFC_NDEF_TNF_MIME_TYPE,
            (const uint8_t *) "text/plain", 10,
            (const uint8_t *) "net.parent", 11,
            (const uint8_t *) addr_str, strlen(addr_str)
                );
    }
}

void net_status_init(
    void)
{
    nfcif_register_handler(&(nfcif_handler_t ) {
            .on_open = net_status_nfc_on_open
        });
}
