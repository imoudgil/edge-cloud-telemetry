#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void copy_str(char *dest, size_t dest_size, const char *src) {
    if (dest_size == 0) {
        return;
    }
    snprintf(dest, dest_size, "%s", src ? src : "");
}

int config_load_from_env(agent_config_t *cfg) {
    const char *endpoint = getenv("TELEMETRY_ENDPOINT");
    const char *interval = getenv("TELEMETRY_INTERVAL_SEC");
    const char *host_id = getenv("TELEMETRY_HOST_ID");

    copy_str(cfg->endpoint, sizeof(cfg->endpoint),
             endpoint ? endpoint : "http://127.0.0.1:8000/api/v1/telemetry");

    cfg->interval_sec = interval ? (unsigned)strtoul(interval, NULL, 10) : 5U;
    if (cfg->interval_sec < 1U) {
        cfg->interval_sec = 1U;
    }

    if (host_id && host_id[0] != '\0') {
        copy_str(cfg->host_id, sizeof(cfg->host_id), host_id);
    } else {
        if (gethostname(cfg->host_id, sizeof(cfg->host_id)) != 0) {
            copy_str(cfg->host_id, sizeof(cfg->host_id), "unknown-host");
        }
    }

    return 0;
}
