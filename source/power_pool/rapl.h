#ifndef RAPL_H
#define RAPL_H
#include <pthread.h>
#include <stdbool.h>
#include "power_client.h"

/**
 * This struct holds a power reading at a specific instance
 */
typedef struct {
    long double nowtime;
    long double power1;
    long double power2;
} power_reading_t;

/**
 * Context struct for the power reading thread. This thread reads
 * power every second and puts the power readings into a circular buffer. These
 * power readings are then read by the local decider, which has a pointer to
 * this power_ctx_t.
 */
typedef struct {
    bool dying;
    power_reading_t *readings; // this is a lock free circular buffer. readings are added and removed via gcc atomic laods and stores
    int len;
} power_ctx_t;

void *read_power(void *args);
power_ctx_t *init_power_ctx(int len);
#endif
