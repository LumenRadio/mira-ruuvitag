#ifndef SENSORS_SENDER_H
#define SENSORS_SENDER_H

#include <mira.h>

#include "sensor-value.h"

typedef struct
{
    mira_net_udp_connection_t *conn;
} sensors_sender_context_t;

void sensors_sender_init(
    sensors_sender_context_t *ctx);

void sensors_sender_send(
    sensors_sender_context_t *ctx,
    const sensor_value_t **values,
    int num_values);

#endif /* SENSORS_SOCKET_H_ */
