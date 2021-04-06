#ifndef RAPL_H
#define RAPL_H
#include <pthread.h>
#include <stdbool.h>
#include "power_client.h"

typedef struct {
    long double nowtime;
    long double power1;
    long double power2;
} power_reading_t;

typedef struct {
    bool dying;
    power_reading_t *readings;
    int len; // len must be a power of 2 
} power_ctx_t;

void *read_power(void *args);
power_ctx_t *init_power_ctx(int len);
#endif