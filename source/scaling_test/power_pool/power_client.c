#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <czmq.h>
#include <bsd/stdlib.h>
#include <math.h>
#include <time.h>

#include <errno.h>
#include <string.h>

#include "power_client.h"
#include "ctx.h"
#include "rapl.h"

#define DEFAULT_FREQUENCY 1
#define HALF_FREQUENCY 1
#define POWER_MARGIN 3
#define HALF_POWER_MARGIN 1
#define MINIMUM_POWER_CAP 30.0
#define MAX_TRANSACTION_AMT 30

void *client_thread(void *args);
double get_power_reading(client_ctx_t *ctx, cpu_t *cpu_list);
void set_power(int id, double power);
double calc_extra_power(double current_power_limit, double current_power);
power_exchange_msg_t *send_power_request(client_ctx_t *data, bool urgency, double necessary_power);
void *reset_socket(client_ctx_t *data, void *socket);

cpu_t create_cpu_t(int id, double powercap)
{
    cpu_t ret;
    ret.id = id;
    ret.initial_power_limit = powercap;
    ret.current_power_limit = powercap;

    ret.available_power = 0.0f;
    ret.urgency = false;
    ret.class = 0;
    ret.release_power = false;
    pthread_mutex_init(&ret.lock, NULL);

    return ret;
}

double max_transaction_amount(double pool_size, double necessary_power)
{
    if (necessary_power > 0)
        return necessary_power;

    if (pool_size > 300)
    {
        return MAX_TRANSACTION_AMT;
    }
    else if (pool_size > 10)
    {
        return (floor(pool_size/10.0f));
    }
    else
    {
        return 1;
    }
}

/**
 * If before runtime has elapsed (defined as command line param) it returns cap.
 * Else it returns 26 (at least 1 POWER_MARGIN below the minimum possible cap of
 * 30W)
 */
double get_power_reading(client_ctx_t *ctx, cpu_t *cpu_list)
{
    time_t t = time(NULL);
    long double nowtime = (long double)t;
    printf("%ld\n", (t - ctx->start_time));
    if ((t - ctx->start_time) < ctx->runtime)
    {
        cpu_list[0].current_power = cpu_list[0].current_power_limit; 
        cpu_list[1].current_power = cpu_list[0].current_power_limit; 
    }
    else
    {
        cpu_list[0].current_power = 26;
        cpu_list[1].current_power = 26;
    }
    return (double)nowtime;
}

// Can't have deciders actually make RAPL calls in simulation
void set_power(int id, double power)
{
    // char cmd[BUFSIZ];
    // if (id == 0)
    // {
    //     sprintf(cmd, "sudo /home/cc/penelope/tools/RAPL/RaplSetPower %f 0 >/dev/null", power);
    // }
    // else
    // {
    //     sprintf(cmd, "sudo /home/cc/penelope/tools/RAPL/RaplSetPower 0 %f >/dev/null", power);
    // }
    // int ret_val = system(cmd);
    // if (ret_val == -1)
    // {
    //     fprintf(stderr, "error setting power via RAPL\n");
    //     exit(-1);
    // }
    return;
}

double calc_extra_power(double current_power_limit, double current_power)
{
    if (current_power <= MINIMUM_POWER_CAP)
        return (current_power_limit - MINIMUM_POWER_CAP);
    else
        return (current_power_limit - current_power - POWER_MARGIN * 0.5);
}

power_exchange_msg_t *send_power_request(client_ctx_t *data, bool urgency, double necessary_power)
{
    void *socket = zmq_socket(data->ctx->context, ZMQ_REQ);
    int timeout = 100;
    zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    int rc, n;
    uint32_t index = arc4random_uniform(data->len);
    assert(index < data->len);
    #ifdef VERBOSE
    printf("sending message to %i, %s\n", index, data->hosts[index]);
    #endif
    char *host = data->hosts[index];
    char endpoint[BUFSIZ];
    sprintf(endpoint, "tcp://%s", host);
    rc = zmq_connect(socket, endpoint);
    if (rc == -1)
    {
        #ifdef VERBOSE
        printf("failed to connect\n");
        #endif
        zmq_close(socket);
        return NULL;
    }

    power_exchange_msg_t request = {necessary_power, urgency};
    zmq_msg_t req_msg, rec_msg;
    rc = zmq_msg_init_data(&req_msg, (void*)(&request), sizeof(request), NULL, NULL); assert(rc == 0);
    n = zmq_msg_send(&req_msg, socket, 0);

    if (n == sizeof(request))
    {
        zmq_msg_init(&rec_msg);
        int rc2 = zmq_msg_recv(&rec_msg, socket, 0);
        if (rc2 == -1)
        {
            printf("failed to receive response %d, %s\n", rc2, strerror(errno));
        }
        rc = zmq_disconnect(socket, endpoint); assert(rc == 0);
        zmq_close(socket);
        return (rc2 == -1) ? NULL : (power_exchange_msg_t *)zmq_msg_data(&rec_msg);
    }
    #ifdef VERBOSE
    printf("failed to send %d, %s\n", n, strerror(errno));
    #endif
    zmq_close(socket);
    return NULL;
}

void* reset_socket(client_ctx_t *data, void *socket)
{
    zmq_close(socket);
    void *sender = zmq_socket(data->ctx->context, ZMQ_REQ);
    int timeout = 10000;
    zmq_setsockopt(sender, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    return sender;
}

void *client_thread(void *args)
{
    sleep(1);
    printf("starting client thread\n");

    client_ctx_t *data = (client_ctx_t *)args;
    void *sender = zmq_socket(data->ctx->context, ZMQ_REQ);
    int timeout = 10000;
    zmq_setsockopt(sender, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    cpu_t *cpu_list = data->ctx->cpus;

    set_power(0, cpu_list[0].initial_power_limit);
    set_power(1, cpu_list[1].initial_power_limit);

    char outfilename[BUFSIZ];
    sprintf(outfilename, "/home/cc/penelope_ttime_%d", data->ctx->id);
    FILE *outfile = fopen(outfilename, "w");

    // keep track of server response times and total number of messages to then
    // print the mean turnaround time at the end of execution
    long unsigned int ttime = 0;
    int num_msgs = 0;

    while (!data->ctx->dying)
    {
        nanosleep(&data->interval, NULL);

        get_power_reading(data, cpu_list);

        for (int i = 0; i < 2; i++)
        {
            if (cpu_list[i].current_power < 0 || cpu_list[i].current_power > 200)
            {
                break;
            }
            
            bool exempt_from_release = false;
            #ifdef VERBOSE
            printf("power: %f, cap: %f\n", 
                    cpu_list[i].current_power,
                    cpu_list[i].current_power_limit);
            #endif
            if (cpu_list[i].current_power < cpu_list[i].current_power_limit - POWER_MARGIN)
            {
                // post extra power
                double extra_power = calc_extra_power(cpu_list[i].current_power_limit, cpu_list[i].current_power);
                if (extra_power > 0)
                {
                    cpu_list[i].next_power_limit = cpu_list[i].current_power_limit - extra_power;
                    set_power(i, cpu_list[i].next_power_limit);
                    cpu_list[i].current_power_limit = cpu_list[i].next_power_limit;
                    exempt_from_release = true; // don't need to release more if we've already released

                    data->ctx->start_ctime = (struct timeval *)malloc(sizeof(struct timeval));
                    gettimeofday(data->ctx->start_ctime, NULL);
                    printf("starting ctime\n"); // when we release power, it is the instant that we start the "power redistribution" clock
                }

				struct timeval stop, start;
				gettimeofday(&start, NULL);

                pthread_mutex_lock(&cpu_list[i].lock);
                cpu_list[i].class = 1;
                cpu_list[i].urgency = false;
                if (extra_power > 0)
                {
                    cpu_list[i].available_power += extra_power;
                }
                
                #ifdef VERBOSE
                printf("Available power (%d): %f\n", i, cpu_list[i].available_power);
                #endif
                pthread_mutex_unlock(&cpu_list[i].lock);


				gettimeofday(&stop, NULL);
				long unsigned int runtime = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
                printf("took %lu us\n", runtime); 
                fprintf(outfile, "%lu\n", runtime);
                ttime += runtime;
                num_msgs += 1;

                #ifdef VERBOSE
                printf("Under powercap. extra_power: %f\n", extra_power);
                #endif
            } 
            else
            {
                // need power
                if (cpu_list[i].current_power_limit >= cpu_list[i].initial_power_limit)
                {
                    // not necessary; randomly request power, but with no urgency
                    // also will yield power to requesting threads
                    pthread_mutex_lock(&cpu_list[i].lock);
                    cpu_list[i].urgency = false;
                    cpu_list[i].class = 3;
                    pthread_mutex_unlock(&cpu_list[i].lock);

					struct timeval stop, start;
					gettimeofday(&start, NULL);

                    double received_power = 0.0f;
                    for (int j = 0; j < 2; j++)
                    {
                        pthread_mutex_lock(&cpu_list[j].lock);
                        double max_size = max_transaction_amount(cpu_list[j].available_power, 0);
                        double delta_power = (cpu_list[j].available_power < max_size) ? cpu_list[j].available_power : max_size;
                        received_power += delta_power;
                        cpu_list[j].available_power -= delta_power;
                        pthread_mutex_unlock(&cpu_list[j].lock);
                    }

                    if (received_power == 0)
                    {
                        // make request
                        power_exchange_msg_t *response = send_power_request(data, false, 0);
                        received_power = (response != NULL) ? response->power_exchanged : 0;
						gettimeofday(&stop, NULL);
                    }

                    if (received_power > 0)
                    {
						gettimeofday(&stop, NULL);
                        cpu_list[i].next_power_limit = cpu_list[i].current_power_limit + received_power;
                        set_power(i, cpu_list[i].next_power_limit);
                        cpu_list[i].current_power_limit = cpu_list[i].next_power_limit;
                    }

					long unsigned int runtime = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec;
					printf("took %lu us\n", runtime); 
                    fprintf(outfile, "%lu\n", runtime);
                    ttime += runtime;
                    num_msgs += 1;
                    #ifdef VERBOSE
                    printf("Need power, above initial cap. exchanged: %f. new limit: %f\n", 
                            received_power, 
                            cpu_list[i].current_power_limit
                            );
                    #endif
                }
                else
                {
                    // necessary; will not yield power. will randomly request with urgency
                    double necessary_power = cpu_list[i].initial_power_limit - cpu_list[i].current_power_limit;
                    #ifdef VERBOSE
                    printf("necessary: %f initial: %f current: %f\n", necessary_power, cpu_list[i].initial_power_limit, cpu_list[i].current_power_limit);
                    #endif
                    pthread_mutex_lock(&cpu_list[i].lock);
                    cpu_list[i].urgency = true;
                    cpu_list[i].class = 3;
                    pthread_mutex_unlock(&cpu_list[i].lock);
                    
					struct timeval stop, start;
					gettimeofday(&start, NULL);

                    double received_power = 0.0f;
                    for (int j = 0; j < 2; j++)
                    {
                        pthread_mutex_lock(&cpu_list[j].lock);
                        double max_size = max_transaction_amount(cpu_list[j].available_power, necessary_power);
                        double delta_power = (cpu_list[j].available_power < max_size) ? cpu_list[j].available_power : max_size;
                        received_power += delta_power;
                        cpu_list[j].available_power -= delta_power;
                        pthread_mutex_unlock(&cpu_list[j].lock);
                    }

                    if (received_power == 0)
                    {
                        // make request
                        power_exchange_msg_t *response = send_power_request(data, true, necessary_power);
                        received_power = (response != NULL) ? response->power_exchanged : 0;
						gettimeofday(&stop, NULL);
                    }

                    if (received_power > 0)
                    {
						gettimeofday(&stop, NULL);
                        cpu_list[i].next_power_limit = cpu_list[i].current_power_limit + received_power;
                        set_power(i, cpu_list[i].next_power_limit);
                        cpu_list[i].current_power_limit = cpu_list[i].next_power_limit;
                    }

                    if (received_power < necessary_power)
                    {
                        exempt_from_release = true; // still urgently need power. our urgency overrides
                    }
					long unsigned int runtime = (stop.tv_sec - start.tv_sec) * 1000000 + stop.tv_usec - start.tv_usec; 
                    printf("took %lu us\n", runtime); 
                    fprintf(outfile, "%lu\n", runtime);
                    ttime += runtime;
                    num_msgs += 1;
                    #ifdef VERBOSE
                    printf("Need power, under initial cap. received: %f, needed: %f, new limit: %f\n", 
                            received_power, 
                            necessary_power,
                            cpu_list[i].current_power_limit
                            );
                    #endif
                }
            }

            pthread_mutex_lock(&cpu_list[i].lock);
            if (!exempt_from_release && 
                cpu_list[i].release_power &&
                (cpu_list[i].current_power_limit > cpu_list[i].initial_power_limit)
            )
            {
                double extra_power = cpu_list[i].current_power_limit - cpu_list[i].initial_power_limit;
                cpu_list[i].current_power_limit = cpu_list[i].current_power_limit - extra_power;
                set_power(i, cpu_list[i].current_power_limit);
                cpu_list[i].available_power += extra_power;
                cpu_list[i].release_power = false;
            }
            pthread_mutex_unlock(&cpu_list[i].lock);
        }
    }

    // print the 
    long double avg = ((long double)ttime) / ((long double)num_msgs);
    printf("total: %lu, num_msgs:%d, avg: %LF\n", ttime, num_msgs, avg);
    fclose(outfile);

    fflush(stdout);
    return NULL;
}
