#include "config.h"
#include "http_post.h"
#include "metrics_cpu.h"
#include "metrics_mem.h"
#include "metrics_temp.h"
#include "telemetry.h"

#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static telemetry_state_t g_state;
static volatile sig_atomic_t g_got_signal = 0;

static void on_signal(int sig) {
    (void)sig;
    g_got_signal = 1;
    g_state.shutdown = true;
}

static void *sender_thread(void *arg) {
    agent_config_t *cfg = (agent_config_t *)arg;
    char json[1024];

    while (!g_state.shutdown) {
        telemetry_snapshot_t snap;
        pthread_mutex_lock(&g_state.lock);
        snap = g_state.snap;
        snap.timestamp_unix = time(NULL);
        g_state.snap.timestamp_unix = snap.timestamp_unix;
        pthread_mutex_unlock(&g_state.lock);

        if (telemetry_json_format(&snap, json, sizeof(json)) <= 0) {
            fprintf(stderr, "edge-telemetry: failed to format JSON\n");
        } else {
            long status = 0;
            if (http_post_json(cfg->endpoint, json, &status) != 0) {
                fprintf(stderr, "edge-telemetry: POST failed (HTTP %ld) -> %s\n", status,
                        cfg->endpoint);
            }
        }

        for (unsigned i = 0; i < cfg->interval_sec && !g_state.shutdown; i++) {
            sleep(1);
        }
    }
    return NULL;
}

int main(void) {
    agent_config_t cfg;
    config_load_from_env(&cfg);

    telemetry_init(&g_state, cfg.host_id);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    pthread_t cpu_t, mem_t, temp_t, send_t;
    pthread_create(&cpu_t, NULL, cpu_sampler_thread, &g_state);
    pthread_create(&mem_t, NULL, mem_sampler_thread, &g_state);
    pthread_create(&temp_t, NULL, temp_sampler_thread, &g_state);
    pthread_create(&send_t, NULL, sender_thread, &cfg);

    fprintf(stderr, "edge-telemetry: host=%s endpoint=%s interval=%us\n", cfg.host_id,
            cfg.endpoint, cfg.interval_sec);

    while (!g_got_signal) {
        sleep(1);
    }

    pthread_join(cpu_t, NULL);
    pthread_join(mem_t, NULL);
    pthread_join(temp_t, NULL);
    pthread_join(send_t, NULL);

    telemetry_shutdown(&g_state);
    return 0;
}
