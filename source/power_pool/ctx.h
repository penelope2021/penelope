/**
 * This file holds struct definitions for contexts relating to the power pool
 * and local decider, and exposes functions for the creation or modification of
 * those structs
 */
#ifndef CTX_H
#define CTX_H

#include "rapl.h"
#include "power_client.h"

/**
 * This is the shared context between the power pool and local decider. It holds
 * information needed for thread management, a ZMQ context for sending and
 * receiving messages, and a list of socket-related information that should be
 * shared.
 */
typedef struct
{
    pthread_t server;
    pthread_t client;
    pthread_t power_reader;
    void *context; // ZMQ context
    bool dying;
    cpu_t *cpus;
} shared_ctx_t;

/**
 * A context for power pool specific elements. This was created to make it easy
 * to add elements specific to the power pool, but for now it holds simply a
 * pointer to the shared context
 */
typedef struct {
    shared_ctx_t *ctx;
} server_ctx_t;

/**
 * A context for local decider specific elements. This holds an array of host
 * IPs for other nodes in the system (as strings), our local IP, the length of
 * the hosts list, a shared context with the power-reading thread (more in
 * rapl.h) 
 */
typedef struct {
    shared_ctx_t *ctx;
    char **hosts; // List does include local IP
    char *host;
    int len;
    power_ctx_t *power_ctx;
} client_ctx_t;

/**
 * Struct holding information necessary for a power transaction between local
 * decider and power pool. 
 *
 * Urgent requests use the power_exchanged field to hold their necessary power
 * when sending to power pools. Power pools set that field to the power returned
 * in the response. 
 */
typedef struct {
    double power_exchanged;
    bool urgency; // indicates if this is an urgent request.
} power_exchange_msg_t;

shared_ctx_t *init_shared();
void update_powercap(shared_ctx_t *shared, double powercap);

server_ctx_t *init_server(shared_ctx_t *s);
client_ctx_t *init_client_ctx(shared_ctx_t *s, power_ctx_t *power_ctx, char **hosts, char *host, int len);
#endif
