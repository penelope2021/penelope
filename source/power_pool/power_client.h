/**
 * Governs functions needed for the local decider, as well as the definition of
 * cpu_t and max_transaction_amount, which are used by both the pool and decider. 
 */

#ifndef POWER_CLIENT_H
#define POWER_CLIENT_H

#include <stdlib.h>

/**
 * This struct contains metadata and actual data about each socket. It holds the
 * initial assigned cap, the current cap, the cap for the next iteration, how
 * much power has been freed by this socket (which the power pool draws from),
 * the urgency state, whether it should release power with 
 */
typedef struct {
    int id;
    double initial_power_limit;
    double current_power_limit;
    double next_power_limit;
    double current_power; // current power consumption

    double available_power; // how much excess is on this socket
    bool urgency;
    int class; // legacy, no longer really used. is now redundant with urgency flag
    bool release_power; // whether this decider should release power. triggered by pool receiving urgent request
    pthread_mutex_t lock; // a lock protecting available power and other metadata, since both decider and pool need to access simultaneously
} cpu_t;

cpu_t create_cpu_t(int id, double powercap);
double max_transaction_amount(double pool_size, double necessary_power);
void *client_thread(void *args);
#endif
