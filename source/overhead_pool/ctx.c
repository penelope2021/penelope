#include <dlfcn.h>
#include <stdint.h>

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <czmq.h>
#include "ctx.h"
#include "power_client.h"

power_ipc_node_t *create_node(double powercap, double ipc)
{
    power_ipc_node_t *ret = malloc(sizeof(power_ipc_node_t));
    ret->powercap = powercap;
    ret->ipc = ipc;
    ret->prev = NULL;
    ret->next = NULL;
    return ret;
}

void insert_node(double powercap, double ipc, power_ipc_list_t *list)
{
    power_ipc_node_t *head = list->head;
    power_ipc_node_t *tmp = head->next;
    while (tmp->next != NULL && tmp->powercap <= powercap)
    {
        // update value if already in list
        if (tmp->powercap == powercap)
        {
            printf("LL: cache hit\n");
            tmp->ipc = ipc;
            return;
        }
        tmp = tmp->next;
    }

    if (tmp->next != NULL)
    {
        double diff_above = tmp->powercap - powercap;
        double diff_below = (tmp->prev->prev != NULL) ? powercap - tmp->prev->powercap : 2;

        if (diff_above <= 1 || diff_below <= 1)
        {
            printf("LL: not different enough, %f %f %f %f\n", diff_above, diff_below, powercap, tmp->powercap);
            return;
        }
    }

    printf("LL: insert\n");

    power_ipc_node_t *node = create_node(powercap, ipc);
    node->prev = tmp->prev;
    tmp->prev->next = node;

    tmp->prev = node;
    node->next = tmp;
}

power_ipc_node_t *lookup(double powercap, power_ipc_list_t *list)
{
    power_ipc_node_t *head = list->head;
    power_ipc_node_t *tmp = head->next;
    while (tmp->next != NULL)
    {
        if (tmp->powercap >= powercap)
        {
            return tmp->prev; 
        }
        tmp = tmp->next;
    }
    return NULL;
}

power_ipc_list_t *create_list()
{
    power_ipc_list_t *ret = malloc(sizeof(power_ipc_list_t));
    ret->head = create_node(-1, -1);
    ret->tail = create_node(-1, -1);

    ret->head->next = ret->tail;
    ret->tail->prev = ret->head;
    return ret;
}

pcm_t *init_pcm_struct()
{
    pcm_t *ret = malloc(sizeof(pcm_t));

	void * handle = dlopen("libpcm.so", RTLD_LAZY);
	if (!handle)
    {
		return NULL;
    }

	ret->pcm_c_init = (int (*)()) dlsym(handle, "pcm_c_init");
	ret->pcm_c_start = (void (*)()) dlsym(handle, "pcm_c_start");
	ret->pcm_c_stop = (void (*)()) dlsym(handle, "pcm_c_stop");
	ret->pcm_c_get_cycles = (uint64_t (*)(uint32_t)) dlsym(handle, "pcm_c_get_cycles");
	ret->pcm_c_get_instr = (uint64_t (*)(uint32_t)) dlsym(handle, "pcm_c_get_instr");

	if (ret->pcm_c_init == NULL || 
        ret->pcm_c_start == NULL || 
        ret->pcm_c_stop == NULL ||
		ret->pcm_c_get_cycles == NULL || 
        ret->pcm_c_get_instr == NULL)
    {
        return NULL;
    }
    ret->pcm_c_init();
    return ret;
}

shared_ctx_t *init_shared()
{
    shared_ctx_t *ret = malloc(sizeof(shared_ctx_t));
    ret->context = zmq_ctx_new();
    ret->dying = false;
    return ret;
}

void update_powercap(shared_ctx_t *shared, double powercap)
{
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
    ret->list = create_list();
    ret->pcm = init_pcm_struct();
    return ret;
}
