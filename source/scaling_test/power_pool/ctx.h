#ifndef CTX_H
#define CTX_H

#include "rapl.h"
#include "power_client.h"

typedef struct 
{
	int (*pcm_c_init)();
	void (*pcm_c_start)();
	void (*pcm_c_stop)();
	uint64_t (*pcm_c_get_cycles)(uint32_t core_id);
	uint64_t (*pcm_c_get_instr)(uint32_t core_id);
} pcm_t;

typedef struct power_ipc_node_t power_ipc_node_t;
struct power_ipc_node_t
{
    double powercap;
    double ipc;
    power_ipc_node_t *prev;
    power_ipc_node_t *next;
};

typedef struct
{
    power_ipc_node_t *head;
    power_ipc_node_t *tail;
    int size;
} power_ipc_list_t;

typedef struct
{
    pthread_t server;
    pthread_t client;
    pthread_t power_reader;
    void *context; // ZMQ context
    bool dying;
    cpu_t *cpus;
    int id;
    struct timeval *start_ctime;
} shared_ctx_t;

typedef struct {
    shared_ctx_t *ctx;
    char *port;
} server_ctx_t;

typedef struct {
    shared_ctx_t *ctx;
    char **hosts;
    char *host;
    int len;
    power_ctx_t *power_ctx;
    power_ipc_list_t *list;
    pcm_t *pcm;
    time_t start_time;
    time_t runtime;
    struct timespec interval;
} client_ctx_t;

typedef struct {
    double power_exchanged;
    bool urgency;
} power_exchange_msg_t;

power_ipc_node_t *create_node(double powercap, double ipc);
void insert_node(double powercap, double ipc, power_ipc_list_t *list);
power_ipc_node_t *lookup(double powercap, power_ipc_list_t *list);

power_ipc_list_t *create_list();
pcm_t *init_pcm_struct();

shared_ctx_t *init_shared(int id);
void update_powercap(shared_ctx_t *shared, double powercap);

server_ctx_t *init_server(shared_ctx_t *s, char *port);
client_ctx_t *init_client_ctx(shared_ctx_t *s, power_ctx_t *power_ctx, char **hosts, char *host, int len, time_t runtime, int interval_msecs);
#endif
