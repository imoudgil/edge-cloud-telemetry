#include "metrics_cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
} cpu_counters_t;

static int read_cpu_counters(cpu_counters_t *out) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) {
        return -1;
    }
    char line[256];
    if (!fgets(line, sizeof(line), f)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    unsigned long long guest = 0;
    unsigned long long guest_nice = 0;
    if (sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
               &out->user, &out->nice, &out->system, &out->idle, &out->iowait, &out->irq,
               &out->softirq, &out->steal, &guest, &guest_nice) < 4) {
        return -1;
    }
    (void)guest;
    (void)guest_nice;
    return 0;
}

static unsigned long long cpu_total(const cpu_counters_t *c) {
    return c->user + c->nice + c->system + c->idle + c->iowait + c->irq + c->softirq +
           c->steal;
}

static unsigned long long cpu_idle(const cpu_counters_t *c) {
    return c->idle + c->iowait;
}

void *cpu_sampler_thread(void *arg) {
    telemetry_state_t *state = (telemetry_state_t *)arg;
    cpu_counters_t prev = {0};
    int have_prev = 0;

    while (!state->shutdown) {
        cpu_counters_t cur;
        if (read_cpu_counters(&cur) == 0 && have_prev) {
            unsigned long long total_d = cpu_total(&cur) - cpu_total(&prev);
            unsigned long long idle_d = cpu_idle(&cur) - cpu_idle(&prev);
            double usage = 0.0;
            if (total_d > 0ULL) {
                usage = 100.0 * (1.0 - ((double)idle_d / (double)total_d));
                if (usage < 0.0) {
                    usage = 0.0;
                }
                if (usage > 100.0) {
                    usage = 100.0;
                }
            }

            pthread_mutex_lock(&state->lock);
            state->snap.cpu_usage_percent = usage;
            pthread_mutex_unlock(&state->lock);
        }
        prev = cur;
        have_prev = 1;
        sleep(1);
    }
    return NULL;
}
