#include "telemetry.h"

#include <stdio.h>
#include <string.h>

void telemetry_init(telemetry_state_t *state, const char *host_id) {
    memset(state, 0, sizeof(*state));
    pthread_mutex_init(&state->lock, NULL);
    snprintf(state->snap.host_id, sizeof(state->snap.host_id), "%s", host_id);
    state->snap.temperature_celsius = -1.0;
}

void telemetry_shutdown(telemetry_state_t *state) {
    state->shutdown = true;
    pthread_mutex_destroy(&state->lock);
}

int telemetry_json_format(const telemetry_snapshot_t *snap, char *buf, size_t buflen) {
    return snprintf(
        buf,
        buflen,
        "{"
        "\"host_id\":\"%s\","
        "\"timestamp_unix\":%ld,"
        "\"cpu_usage_percent\":%.2f,"
        "\"mem_total_kb\":%lu,"
        "\"mem_available_kb\":%lu,"
        "\"mem_used_percent\":%.2f,"
        "\"temperature_celsius\":%.2f"
        "}",
        snap->host_id,
        (long)snap->timestamp_unix,
        snap->cpu_usage_percent,
        snap->mem_total_kb,
        snap->mem_available_kb,
        snap->mem_used_percent,
        snap->temperature_celsius);
}
