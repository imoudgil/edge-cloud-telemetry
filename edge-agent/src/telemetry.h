#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define TELEM_HOST_ID_MAX 256

typedef struct {
    char host_id[TELEM_HOST_ID_MAX];
    time_t timestamp_unix;
    double cpu_usage_percent;
    unsigned long mem_total_kb;
    unsigned long mem_available_kb;
    double mem_used_percent;
    double temperature_celsius;
} telemetry_snapshot_t;

typedef struct {
    telemetry_snapshot_t snap;
    pthread_mutex_t lock;
    volatile bool shutdown;
} telemetry_state_t;

void telemetry_init(telemetry_state_t *state, const char *host_id);
void telemetry_shutdown(telemetry_state_t *state);

int telemetry_json_format(const telemetry_snapshot_t *snap, char *buf, size_t buflen);

#endif
