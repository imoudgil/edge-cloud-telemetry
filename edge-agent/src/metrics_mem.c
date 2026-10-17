#include "metrics_mem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned long parse_kb_line(const char *line) {
    unsigned long val = 0;
    if (sscanf(line, "%*s %lu kB", &val) == 1) {
        return val;
    }
    return 0;
}

static int read_meminfo(unsigned long *total_kb, unsigned long *avail_kb) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) {
        return -1;
    }

    char line[256];
    unsigned long total = 0;
    unsigned long avail = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            total = parse_kb_line(line);
        } else if (strncmp(line, "MemAvailable:", 13) == 0) {
            avail = parse_kb_line(line);
        }
        if (total > 0 && avail > 0) {
            break;
        }
    }
    fclose(f);

    if (total == 0) {
        return -1;
    }
    *total_kb = total;
    *avail_kb = avail;
    return 0;
}

void *mem_sampler_thread(void *arg) {
    telemetry_state_t *state = (telemetry_state_t *)arg;

    while (!state->shutdown) {
        unsigned long total = 0;
        unsigned long avail = 0;
        if (read_meminfo(&total, &avail) == 0) {
            double used_pct = 0.0;
            if (total > 0) {
                unsigned long used = total - avail;
                used_pct = 100.0 * ((double)used / (double)total);
            }

            pthread_mutex_lock(&state->lock);
            state->snap.mem_total_kb = total;
            state->snap.mem_available_kb = avail;
            state->snap.mem_used_percent = used_pct;
            pthread_mutex_unlock(&state->lock);
        }
        sleep(2);
    }
    return NULL;
}
