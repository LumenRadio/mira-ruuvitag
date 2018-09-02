#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <stdint.h>

typedef struct {
    char name[32];

    uint8_t net_key[16];
    uint32_t net_panid;
    uint8_t net_rate;

    uint8_t reserved;

    uint16_t update_interval;
} app_config_t;

extern app_config_t app_config;

void app_config_init(void);
int app_config_is_configured(void);

#endif
