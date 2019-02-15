/*----------------------------------------------------------------------------
Copyright (c) 2017 LumenRadio AB
This code is the property of Lumenradio AB and may not be redistributed in any
form without prior written permission from LumenRadio AB.

This example is provided as is, without warranty.
----------------------------------------------------------------------------*/

#include <mira.h>
#include <stdio.h>
#include <string.h>



#define UDP_PORT 7338

#define SEND_INTERVAL 60
#define STATE_UPDATE_INTERVAL 5
#define SENSOR_VALUE_TYPE_ETX 5
#define SENSOR_VALUE_TYPE_CLOCK_DRIFT 6

// 832
/*
#define LED1_PIN MIRA_GPIO_PIN(0, 17)
#define LED2_PIN MIRA_GPIO_PIN(0, 18)
#define LED3_PIN MIRA_GPIO_PIN(0, 19)
#define LED4_PIN MIRA_GPIO_PIN(0, 20)
*/

// 840
#define LED1_PIN MIRA_GPIO_PIN(0, 13)
#define LED2_PIN MIRA_GPIO_PIN(0, 14)
#define LED3_PIN MIRA_GPIO_PIN(0, 15)
#define LED4_PIN MIRA_GPIO_PIN(0, 16)


//static const uint32_t pan_id = 0x83ae13a3;
static const mira_net_config_t net_config = {
    .pan_id = 0x9432165d,
    .key = {
        0xfd, 0x41, 0x09, 0xe0,
        0x43, 0xac, 0x1e, 0xb7,
        0x1a, 0xa9, 0xf3, 0x0a,
        0xfa, 0x89, 0x48, 0xd3
    },
    .mode = MIRA_NET_MODE_MESH,
    .rate = MIRA_NET_RATE_FAST,
    .antenna = MIRA_NET_ANTENNA_ONBOARD,
    .prefix = NULL /* default prefix */
};

static mira_net_state_t net_state;
extern int32_t drift_ppm;


/*
 * Identifies as a node.
 * Sends data to the root.
 */

PROCESS(state_proc, "State process");
PROCESS(main_proc, "Main process");


MIRA_IODEFS(
    MIRA_IODEF_NONE,    /* fd 0: stdin */
    MIRA_IODEF_UART(0), /* fd 1: stdout */
    MIRA_IODEF_NONE     /* fd 2: stderr */
    /* More file descriptors can be added, for use with dprintf(); */
);

//static uint8_t led_state = 0;

void mira_setup(
    void)
{

    mira_status_t uart_ret;
    mira_uart_config_t uart_config = {
        .baudrate = 115200,
        .tx_pin = MIRA_GPIO_PIN(0, 6),
        .rx_pin = MIRA_GPIO_PIN(0, 8)
    };

    mira_gpio_set_dir(LED1_PIN, MIRA_GPIO_DIR_OUT);
    mira_gpio_set_dir(LED2_PIN, MIRA_GPIO_DIR_OUT);
    mira_gpio_set_dir(LED3_PIN, MIRA_GPIO_DIR_OUT);
    mira_gpio_set_dir(LED4_PIN, MIRA_GPIO_DIR_OUT);

    mira_gpio_set_value(LED1_PIN, 1);
    mira_gpio_set_value(LED2_PIN, 1);
    mira_gpio_set_value(LED3_PIN, 1);
    mira_gpio_set_value(LED4_PIN, 1);


    uart_ret = mira_uart_init(0, &uart_config);
    if (uart_ret != MIRA_SUCCESS) {
        /* Nowhere to send an error message */
    }

    process_start(&main_proc, NULL);
}



static void udp_listen_callback(
    mira_net_udp_connection_t *connection,
    const void *data,
    uint16_t data_len,
    const mira_net_udp_callback_metadata_t *metadata,
    void *storage)
{
    char root_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    uint16_t i;

    //led_state = !led_state;
    //mira_gpio_set_value(LED4_PIN, led_state);
    //printf("led_state: %d\n",led_state);


    printf("Received message from [%s]:%u: ",
        mira_net_toolkit_format_address(root_address_str, metadata->source_address),
        metadata->source_port);

    for (i = 0; i < data_len - 1; i++) {
        printf("%c", ((char *) data)[i]);
    }
    printf("\n");
}

PROCESS_THREAD(state_proc, ev, data)
{
    static struct etimer timer;
    static uint8_t led_state2 = 0;
    static mira_net_address_t root_address;
    static char root_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    static mira_net_address_t parent_address;
    static char parent_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    static mira_link_metric_t link_metric;
    static uint8_t etx;

    PROCESS_BEGIN();

    PROCESS_PAUSE();

    while (1) {
        net_state = mira_net_get_state();

        //LED on when joined
        if ( net_state == MIRA_NET_STATE_JOINED) {
            printf("Joined!\n");
            mira_gpio_set_value(LED2_PIN, 0);
            mira_gpio_set_value(LED3_PIN, 0);
        }

        //LED blinking when associated
        else if (net_state == MIRA_NET_STATE_ASSOCIATED) {
            printf("Associated\n");
            mira_gpio_set_value(LED2_PIN, 0);
            mira_gpio_set_value(LED3_PIN, 0);

            etimer_set(&timer, CLOCK_SECOND);
            PROCESS_YIELD_UNTIL(etimer_expired(&timer));


            mira_gpio_set_value(LED2_PIN, 1);
            mira_gpio_set_value(LED3_PIN, 1);

        //LED off when not associated
        } else {
            printf("Not associated\n");

            led_state2 = !led_state2;
            if (led_state2) {
                mira_gpio_set_value(LED2_PIN, 1);
                mira_gpio_set_value(LED3_PIN, 0);

            } else {
                mira_gpio_set_value(LED2_PIN, 0);
                mira_gpio_set_value(LED3_PIN, 1);

            }
        }

        link_metric = mira_net_get_parent_link_metric();
        etx = link_metric >> 7;

        mira_net_get_root_address(&root_address);
        mira_net_get_parent_address(&parent_address);
        mira_net_toolkit_format_address(root_address_str, &root_address);
        mira_net_toolkit_format_address(parent_address_str, &parent_address);

        // Display if root is parent -> LED1 is lit.
        if (0 == memcmp(&root_address, &parent_address, sizeof(mira_net_address_t))) {
            mira_gpio_set_value(LED1_PIN, 0); // on
            printf("Parent is root\n");
        } else {
            mira_gpio_set_value(LED1_PIN, 1); // off
            printf("Parent is NOT root\n");
        }

            printf("ETX: %d\nETX >> 7: %d\n", link_metric, etx);
        if (etx < 2) {
            mira_gpio_set_value(LED4_PIN, 0);
        } else {
            mira_gpio_set_value(LED4_PIN, 1);
        }

        etimer_set(&timer, STATE_UPDATE_INTERVAL*CLOCK_SECOND);
        PROCESS_YIELD_UNTIL(etimer_expired(&timer));

    }

    PROCESS_END();
}



PROCESS_THREAD(main_proc, ev, data)
{
    static struct etimer timer;
    static mira_net_udp_connection_t *udp_connection;
    static mira_net_address_t root_address;
    static char root_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    static mira_net_address_t parent_address;
    static char parent_address_str[MIRA_NET_MAX_ADDRESS_STR_LEN];
    static mira_status_t res;
    static int16_t power_cbm;
    static int32_t link_metric;
    static uint32_t link_metric_fix_point;
    static int32_t clock_drift;
    static uint32_t clock_drift_fix_point;
    static uint8_t etx;

    //static const char *message = "ping\n";

    static mira_status_t res1;
    static mira_status_t res2;
    static mira_status_t res3;
    static mira_status_t res4;

    static char name[32];
    memset(name, 0, 32);
    memcpy(name, "repeater", strlen("repeater"));


    PROCESS_BEGIN();
    /* Pause once, so we don't run anything before finish of startup */
    PROCESS_PAUSE();

    printf("Starting Node (Sender).\n");
    printf("Sending one packet every %d seconds\n", SEND_INTERVAL);

    /*
     * Set up the Mira Network with a given PAN ID and encryption key.
     */


    res1 = mira_net_init(&net_config);
    if (res1 != MIRA_SUCCESS) {
        printf("ERROR: mira_net_init\n");
        //mira_gpio_set_value(LED1_PIN, 0);
    }

    res2 = mira_net_set_txpower(80);
    if (res2 != MIRA_SUCCESS) {
        printf("ERROR: mira_net_set_txpower\n");
        //mira_gpio_set_value(LED1_PIN, 0);
    }
    res3 = mira_net_set_high_power();
    if (res3 != MIRA_SUCCESS) {
        printf("ERROR: mira_net_set_high_power\n");
        //mira_gpio_set_value(LED1_PIN, 0);
    }

    res4 = mira_net_get_actual_txpower(&power_cbm);
    if (res4 != MIRA_SUCCESS) {
        printf("ERROR: mira_net_get_actual_txpower\n");
        //mira_gpio_set_value(LED1_PIN, 0);
    }
    printf("TX-power: %d\n",power_cbm );


    /*
     * Open a connection, but don't specify target address yet, which means
     * only mira_net_udp_send_to() can be used to send packets later.
     */
    udp_connection = mira_net_udp_bind(NULL, UDP_PORT, UDP_PORT, udp_listen_callback, NULL);


    process_start(&state_proc, NULL);

    link_metric_fix_point = 128;
    clock_drift_fix_point = 256000000;


    while (1) {

        uint8_t payload[160];
        int payload_len;
        payload_len = 0;



        etimer_set(&timer, SEND_INTERVAL * CLOCK_SECOND);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

        /* Try to retrieve the root address. */
        res = mira_net_get_root_address(&root_address);
        mira_net_get_parent_address(&parent_address);
        mira_net_toolkit_format_address(root_address_str, &root_address);
        memset(parent_address_str,0, MIRA_NET_MAX_ADDRESS_STR_LEN);
        mira_net_toolkit_format_address(parent_address_str, &parent_address);

        link_metric = mira_net_get_parent_link_metric();
        etx = link_metric >> 7;

        clock_drift = drift_ppm;


        memcpy(&payload[payload_len], name, 32);
        payload_len += 32;
        memcpy(&payload[payload_len], &parent_address_str, MIRA_NET_MAX_ADDRESS_STR_LEN);
        payload_len += MIRA_NET_MAX_ADDRESS_STR_LEN;

        payload[payload_len+0] = SENSOR_VALUE_TYPE_ETX;

        payload[payload_len+1] = (link_metric >> 24) & 0xff;
        payload[payload_len+2] = (link_metric >> 16) & 0xff;
        payload[payload_len+3] = (link_metric >>  8) & 0xff;
        payload[payload_len+4] = (link_metric >>  0) & 0xff;

        payload[payload_len+5] = (link_metric_fix_point >> 24) & 0xff;
        payload[payload_len+6] = (link_metric_fix_point >> 16) & 0xff;
        payload[payload_len+7] = (link_metric_fix_point >>  8) & 0xff;
        payload[payload_len+8] = (link_metric_fix_point >>  0) & 0xff;
        payload_len += 9;

        payload[payload_len+0] = SENSOR_VALUE_TYPE_CLOCK_DRIFT;

        payload[payload_len+1] = (clock_drift >> 24) & 0xff;
        payload[payload_len+2] = (clock_drift >> 16) & 0xff;
        payload[payload_len+3] = (clock_drift >>  8) & 0xff;
        payload[payload_len+4] = (clock_drift >>  0) & 0xff;

        payload[payload_len+5] = (clock_drift_fix_point >> 24) & 0xff;
        payload[payload_len+6] = (clock_drift_fix_point >> 16) & 0xff;
        payload[payload_len+7] = (clock_drift_fix_point >>  8) & 0xff;
        payload[payload_len+8] = (clock_drift_fix_point >>  0) & 0xff;
        payload_len += 9;




        /*
        * If root address is successfully retrieved, send a message
        * to the root node on the given UDP Port.
        * Close the connection after the message is sent.
        */
        if (res != MIRA_SUCCESS) {
            printf("Waiting for root address\n");
        } else {


           printf("Sending to address: %s\n * parent: %s\n * ETX(>>7): %d\n",
                root_address_str,
                parent_address_str,
                etx);


            mira_net_udp_send_to(udp_connection, &root_address, UDP_PORT, payload, payload_len);

        }



    }

    mira_net_udp_close(udp_connection);

    PROCESS_END();
}
