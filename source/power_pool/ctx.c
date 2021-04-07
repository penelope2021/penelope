#include <dlfcn.h>
#include <stdint.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <czmq.h>
#include "ctx.h"
#include "power_client.h"

shared_ctx_t *init_shared()
{
    shared_ctx_t *ret = malloc(sizeof(shared_ctx_t));
    ret->context = zmq_ctx_new(); // creates new ZMQ context
    ret->dying = false;
    return ret;
}

void update_powercap(shared_ctx_t *shared, double powercap)
{
    // see  power_client.h for more details on cpu_t
    shared->cpus = malloc(sizeof(cpu_t) * 2);
    shared->cpus[0] = create_cpu_t(0, powercap);
    shared->cpus[1] = create_cpu_t(1, powercap);
}

server_ctx_t *init_server(shared_ctx_t *s) 
{
    server_ctx_t *ret = malloc(sizeof(server_ctx_t));
    ret->ctx = s;
    return ret;
}

client_ctx_t *init_client_ctx(shared_ctx_t *s, power_ctx_t *power_ctx, char **hosts, char *host, int len)
{
    client_ctx_t *ret = malloc(sizeof(client_ctx_t));
    ret->ctx = s;
    ret->hosts = hosts;
    ret->host = malloc(sizeof(char) * strlen(host));
    strcpy(ret->host, host);
    ret->len = len;
    ret->power_ctx = power_ctx;
    return ret;
}
