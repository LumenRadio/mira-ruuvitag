#include <mira.h>
#include <stdio.h>

#include "app-config.h"
#include "nfc-if.h"
#include "application.h"

#define USE_UART 0

MIRA_IODEFS(
    MIRA_IODEF_NONE,
#if USE_UART
    MIRA_IODEF_UART(0),
#else
    MIRA_IODEF_NONE,
#endif
    MIRA_IODEF_NONE,
);

PROCESS(boot_proc, "Boot process");

void mira_setup(
    void)
{
#if USE_UART
    mira_uart_init(0, &(mira_uart_config_t ) {
            .baudrate = 115200,
            .tx_pin = 6,
            .rx_pin = 8
        });
#endif
    process_start(&boot_proc, NULL);
}

PROCESS_THREAD(boot_proc, ev, data)
{
    static struct etimer timer;

    PROCESS_BEGIN();
    PROCESS_PAUSE();

    nfcif_init();
    app_config_init();

    if(app_config_is_configured()) {
        printf("Configured, starting\n");
        process_start(&main_proc, NULL);
    } else {
        printf("Not configured, sleeping\n");
    }

    while (1) {
        etimer_set(&timer, CLOCK_SECOND);
        PROCESS_YIELD_UNTIL(etimer_expired(&timer));
    }

    PROCESS_END();
}
