#include "metrics_temp.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int read_zone_temp(const char *zone_path, double *celsius_out) {
    char type_path[512];
    char temp_path[512];
    snprintf(type_path, sizeof(type_path), "%s/type", zone_path);
    snprintf(temp_path, sizeof(temp_path), "%s/temp", zone_path);

    FILE *tf = fopen(type_path, "r");
    if (!tf) {
        return -1;
    }
    char type[64] = {0};
    if (!fgets(type, sizeof(type), tf)) {
        fclose(tf);
        return -1;
    }
    fclose(tf);

    /* Prefer CPU package sensor when present. */
    if (strstr(type, "x86_pkg_temp") == NULL && strstr(type, "cpu") == NULL &&
        strstr(type, "acpitz") == NULL) {
        /* Still allow thermal_zone0 as fallback later. */
    }

    FILE *tempf = fopen(temp_path, "r");
    if (!tempf) {
        return -1;
    }
    long milli = -1;
    if (fscanf(tempf, "%ld", &milli) != 1 || milli < 0) {
        fclose(tempf);
        return -1;
    }
    fclose(tempf);

    *celsius_out = (double)milli / 1000.0;
    return 0;
}

static int read_best_temperature(double *celsius_out) {
    const char *base = "/sys/class/thermal";
    DIR *dir = opendir(base);
    if (!dir) {
        return -1;
    }

    double best = -1.0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, "thermal_zone", 12) != 0) {
            continue;
        }
        char zone_path[512];
        snprintf(zone_path, sizeof(zone_path), "%s/%s", base, ent->d_name);

        char type_path[512];
        snprintf(type_path, sizeof(type_path), "%s/type", zone_path);
        FILE *tf = fopen(type_path, "r");
        if (!tf) {
            continue;
        }
        char type[64] = {0};
        fgets(type, sizeof(type), tf);
        fclose(tf);

        double c = -1.0;
        if (read_zone_temp(zone_path, &c) != 0) {
            continue;
        }

        if (strstr(type, "x86_pkg_temp") || strstr(type, "acpitz") || strstr(type, "cpu")) {
            *celsius_out = c;
            closedir(dir);
            return 0;
        }
        if (best < 0.0) {
            best = c;
        }
    }
    closedir(dir);

    if (best >= 0.0) {
        *celsius_out = best;
        return 0;
    }
    return -1;
}

void *temp_sampler_thread(void *arg) {
    telemetry_state_t *state = (telemetry_state_t *)arg;

    while (!state->shutdown) {
        double celsius = -1.0;
        if (read_best_temperature(&celsius) != 0) {
            celsius = -1.0;
        }

        pthread_mutex_lock(&state->lock);
        state->snap.temperature_celsius = celsius;
        pthread_mutex_unlock(&state->lock);

        sleep(3);
    }
    return NULL;
}
