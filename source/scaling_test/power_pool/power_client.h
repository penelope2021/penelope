#ifndef POWER_CLIENT_H
#define POWER_CLIENT_H

#include <stdlib.h>

typedef struct {
    int id;
    double initial_power_limit;
    double current_power_limit;
    double next_power_limit;
    double current_power;

    double available_power;
    bool urgency;
    int class;
    bool release_power;
    pthread_mutex_t lock;
} cpu_t;

cpu_t create_cpu_t(int id, double powercap);
double max_transaction_amount(double pool_size, double necessary_power);
void *client_thread(void *args);
#endif
