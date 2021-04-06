#define _GNU_SOURCE
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <czmq.h>
#include <errno.h>
#include "power_client.h"
#include "ctx.h"

void *server_thread(void *args);

void *server_thread(void *args)
{
    printf("starting server\n");
    server_ctx_t *data = (server_ctx_t *)args;
    void *responder = zmq_socket(data->ctx->context, ZMQ_REP);
    char addr[BUFSIZ];
    sprintf(addr, "tcp://*:%s", data->port);
    printf("%s\n", addr);
    int rc = zmq_bind(responder, addr);
    assert(rc == 0);
    struct timeval stop, start;
    gettimeofday(&start, NULL);

    char filename[BUFSIZ];
    sprintf(filename, "/home/cc/penelope_pool_%d", data->ctx->id);
    FILE *pool_file = fopen(filename, "w");

    double max_pool_size = 2*(data->ctx->cpus[0].initial_power_limit - 30);

    // zmq_pollitem_t items[1];
    // items[0].socket = responder;
    // items[0].events = ZMQ_POLLIN;

    zmq_pollitem_t items[] = {
        {responder, 0, ZMQ_POLLIN, 0}
    };

    cpu_t *cpu_list = data->ctx->cpus;
    double tot_power_given = 0.0f;
    while (!data->ctx->dying) 
    {
        zmq_poll(items, 1, 1000);
        if (items[0].revents & ZMQ_POLLIN)
        {
            zmq_msg_t req_msg, resp_msg;
            zmq_msg_init(&req_msg);

            if (zmq_msg_recv(&req_msg, responder, 0) == -1) 
            {
                exit(-1);
            }
            power_exchange_msg_t *request = (power_exchange_msg_t *)zmq_msg_data(&req_msg);
            zmq_msg_close(&req_msg);
            double tot_avail = 0.0f;
            double power_to_give = 0.0f;
            for (int i = 0; i < 2; i++)
            {
                pthread_mutex_lock(&cpu_list[i].lock);
                tot_avail += cpu_list[i].available_power;
                if (cpu_list[i].available_power > 0)
                {
                    // power_exchanged initially represents the necessary power (proxy for urgency)
                    double max_size = max_transaction_amount(cpu_list[i].available_power, request->power_exchanged);
                    double delta_power = (cpu_list[i].available_power < max_size) ? cpu_list[i].available_power : max_size;
                    power_to_give += delta_power;
                    tot_power_given += delta_power;
                    cpu_list[i].available_power -= delta_power;
                }
                else 
                {
                    if (request->urgency && !cpu_list[i].urgency)
                    {
                        cpu_list[i].release_power = true;
                    }
                }
                pthread_mutex_unlock(&cpu_list[i].lock);
            }
            power_exchange_msg_t response = {power_to_give, 0};
            zmq_msg_init_data(&resp_msg, (void*)(&response), sizeof(response), NULL, NULL);
            int n = zmq_msg_send(&resp_msg, responder, 0); assert(n == sizeof(response));
            #ifdef VERBOSE
            printf("POWER POOL gave %fW\n", power_to_give);
            printf("URGENCY RECEIVED: %d\n", request->urgency);
            #endif

            gettimeofday(&stop, NULL);
            long unsigned int current = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
            double cur_time = ((double)current) / 1000000.0f;
            fprintf(pool_file, "%f,%f\n", cur_time, tot_avail);

            // if power has been given, we must've started
            // so don't need to check if data->ctx->start_ctime != NULL
            if (tot_power_given >= max_pool_size)
            {
                struct timeval *start_ctime = data->ctx->start_ctime;
                long unsigned int ctime = (stop.tv_sec - start_ctime->tv_sec) * 1000000 + 
                    stop.tv_usec - start_ctime->tv_usec;
                double ctime_seconds = ((double)ctime / 1000000.0f);

                char fn[BUFSIZ];
                sprintf(fn, "/home/cc/penelope_ctime_%d", data->ctx->id);
                FILE *ctime_file = fopen(fn, "w");
                fprintf(ctime_file, "%f", ctime_seconds);
                fclose(ctime_file);

                tot_power_given = -1;
            }
        }
        if (access("/home/cc/END_PENELOPE", F_OK) == 0)
            data->ctx->dying = true;
    }
    fflush(pool_file);
    fclose(pool_file);
    pthread_join(data->ctx->client, NULL);
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 8) 
    {
        fprintf(stderr, "not enough arguments provided\n");
        exit(-1);
    }

    char *hostfilename = argv[1];
    int num_hosts = atoi(argv[2]);
    double powercap = atof(argv[3]);
    char *cur_host = argv[4]; // IP of current node. to ensure node doesn't make spurious requests to itself
    char *port = argv[5];
    int r = atoi(argv[6]);
    int id = atoi(argv[7]);
    int frequency = atoi(argv[8]); // time in milliseconds for sleeping time

    char host_port[BUFSIZ];
    sprintf(host_port, "%s:%s", cur_host, port);

    time_t runtime = r;

    char **hosts = malloc(sizeof(char*) * (num_hosts - 1));
    int count = 0;
    FILE *hostfile = fopen(hostfilename, "r");
    if (hostfile == NULL)
    {
        fprintf(stderr, "failed to open hostfile\n");
        exit(-1);
    }

    size_t n;
    while ((count < (num_hosts - 1)) && (getline(&hosts[count], &n, hostfile) != -1))
    {
        int len = strlen(hosts[count]);
        if (hosts[count][len-1] == '\n')
            hosts[count][len-1] = '\0';
        if (strcmp(hosts[count], host_port))
        {
            count++;
        }
    } 
    if (count != (num_hosts - 1))
    {
        fprintf(stderr, "Issue reading in hosts from file\n");
        exit(-1);
    }
    printf("hello world\n");

    power_ctx_t *power_ctx = init_power_ctx(2);
    shared_ctx_t *shared_ctx = init_shared(id);

    // if (pthread_create(&shared_ctx->power_reader, NULL, read_power, (void*)power_ctx) != 0)
    // {
    //     fprintf(stderr, "failed to create server thread\n");
    //     exit(-1);
    // }

    // void *wait_for_power_pool = zmq_socket(shared_ctx->context, ZMQ_REP);
    // int rc = zmq_bind(wait_for_power_pool, "tcp://*:1111");
    // assert(rc == 0);

    // char start[15];
    // int nbytes = zmq_recv(wait_for_power_pool, (void*)start, 15, 0);
    // if (nbytes == -1)
    // {
    //     fprintf(stderr, "error waiting on signal socket used to start power pool\n");
    //     exit(-1);
    // }
    // zmq_send(wait_for_power_pool, "ack", 3, 0);
    // start[nbytes] = '\0';
    // double powercap = atof(start);

    printf("beginning power pool\n");
    update_powercap(shared_ctx, powercap);

    server_ctx_t *server_ctx = init_server(shared_ctx, port);
    client_ctx_t *client_ctx = init_client_ctx(shared_ctx, power_ctx, hosts, cur_host, (num_hosts-1), runtime, frequency);

    // if (pthread_create(&shared_ctx->server, NULL, server_thread, (void *)server_ctx) != 0)
    // {
    //     fprintf(stderr, "failed to create server thread\n");
    //     exit(-1);
    // }
    if (pthread_create(&shared_ctx->client, NULL, client_thread, (void *)client_ctx) != 0)
    {
        fprintf(stderr, "failed to create server thread\n");
        exit(-1);
    }

    // char end[15];
    // if (zmq_recv(wait_for_power_pool, (void*)end, 15, 0) == -1)
    // {
    //     fprintf(stderr, "error waiting on signal socket used to kill power pool\n");
    //     exit(-1);
    // }
    // zmq_send(wait_for_power_pool, "ack", 3, 0);

    // shared_ctx->dying = true;
    // power_ctx->dying = true;

    // pthread_join(shared_ctx->power_reader, NULL);
    // pthread_join(shared_ctx->server, NULL);
    // pthread_join(shared_ctx->client, NULL);
    server_thread((void*)server_ctx);

    return 0;
}
