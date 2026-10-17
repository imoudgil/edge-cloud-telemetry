#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

typedef struct {
    char endpoint[512];
    unsigned interval_sec;
    char host_id[256];
} agent_config_t;

int config_load_from_env(agent_config_t *cfg);

#endif
