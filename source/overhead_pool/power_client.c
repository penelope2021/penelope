#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <czmq.h>
#include <bsd/stdlib.h>
#include <math.h>

#include <errno.h>
#include <string.h>

#include "power_client.h"
#include "ctx.h"
#include "rapl.h"

#define DEFAULT_FREQUENCY 2
#define HALF_FREQUENCY 2
#define POWER_MARGIN 3
#define HALF_POWER_MARGIN 1
#define MINIMUM_POWER_CAP 30.0
#define MAX_TRANSACTION_AMT 30

void *client_thread(void *args);
double get_power_reading(power_ctx_t *ctx, cpu_t *cpu_list);
void set_power(int id, double power);
double calc_extra_power(double current_power_limit, double current_power);
power_exchange_msg_t *send_power_request(client_ctx_t *data, void *socket, bool urgency, double necessary_power);

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

double get_power_reading(power_ctx_t *ctx, cpu_t *cpu_list)
{
    long double power1 = 0;
    long double power2 = 0;
    long double nowtime = 0;
    double num_valid = 0;
    for (int i = 0; i < ctx->len; i++)
    {
        power_reading_t tmp;
        __atomic_load(&ctx->readings[i], &tmp, __ATOMIC_SEQ_CST);
        num_valid = (tmp.nowtime > 0) ? num_valid + 1 : num_valid;
        #ifdef VERBOSE
        if (tmp.nowtime > nowtime)
            printf("%LF %LF %LF\n", tmp.nowtime, tmp.power1, tmp.power2);
        #endif
        nowtime = (tmp.nowtime > nowtime) ? tmp.nowtime : nowtime;
        power1 += tmp.power1;
        power2 += tmp.power2;
    }
    #ifdef VERBOSE
        printf("sum1: %f sum2: %f valid: %f\n", (double)power1, (double)power2, num_valid);
    #endif
    cpu_list[0].current_power = (num_valid > 0) ? (power1 / num_valid) : 0;
    cpu_list[1].current_power = (num_valid > 0) ? (power2 / num_valid) : 0;
    return (double)nowtime;
}

void set_power(int id, double power)
{
    char cmd[BUFSIZ];
    if (id == 0)
    {
        sprintf(cmd, "sudo /home/cc/penelope/tools/RAPL/RaplSetPower %f 0 >/dev/null", power);
    }
    else
    {
        sprintf(cmd, "sudo /home/cc/penelope/tools/RAPL/RaplSetPower 0 %f >/dev/null", power);
    }
    int ret_val = system(cmd);
    if (ret_val == -1)
    {
        fprintf(stderr, "error setting power via RAPL\n");
        exit(-1);
    }
}

double calc_extra_power(double current_power_limit, double current_power)
{
    if (current_power <= MINIMUM_POWER_CAP)
        return (current_power_limit - MINIMUM_POWER_CAP);
    else
        return (current_power_limit - current_power - POWER_MARGIN * 0.5);
}

power_exchange_msg_t *send_power_request(client_ctx_t *data, void *socket, bool urgency, double necessary_power)
{
    return NULL;
    // int rc, n;
    // uint32_t index = arc4random_uniform(data->len);
    // assert(index < data->len);
    // #ifdef VERBOSE
    // printf("sending message to %i, %s\n", index, data->hosts[index]);
    // #endif
    // char *host = data->hosts[index];
    // char endpoint[BUFSIZ];
    // sprintf(endpoint, "tcp://%s:1600", host);
    // rc = zmq_connect(socket, endpoint);
    // if (rc == -1)
    // {
    //     #ifdef VERBOSE
    //     printf("failed to connect\n");
    //     #endif
    //     return NULL;
    // }

    // power_exchange_msg_t request = {necessary_power, urgency};
    // zmq_msg_t req_msg, rec_msg;
    // rc = zmq_msg_init_data(&req_msg, (void*)(&request), sizeof(request), NULL, NULL); assert(rc == 0);
    // n = zmq_msg_send(&req_msg, socket, 0);

    // if (n == sizeof(request))
    // {
    //     // int timeout = 1000;
    //     // zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    //     zmq_msg_init(&rec_msg);
    //     int rc2 = zmq_msg_recv(&rec_msg, socket, 0);
    //     rc = zmq_disconnect(socket, endpoint); assert(rc == 0);
    //     #ifdef VERBOSE
    //     if (rc2 == -1)
    //         printf("failed to receive response\n");
    //     #endif
    //     return (rc2 == -1) ? NULL : (power_exchange_msg_t *)zmq_msg_data(&rec_msg);
    // }
    // #ifdef VERBOSE
    // printf("failed to send %d, %s\n", n, strerror(errno));
    // #endif
    // return NULL;
}

void *client_thread(void *args)
{
    printf("starting client thread\n");
    uint32_t frequency = DEFAULT_FREQUENCY;

    client_ctx_t *data = (client_ctx_t *)args;
    power_ctx_t *power_ctx = data->power_ctx;
    void *sender = zmq_socket(data->ctx->context, ZMQ_REQ);
    int timeout = 10000;
    zmq_setsockopt(sender, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    cpu_t *cpu_list = data->ctx->cpus;
    // pcm_t *pcm = data->pcm;
    // power_ipc_list_t *ipc_list = data->list;

    set_power(0, cpu_list[0].initial_power_limit);
    set_power(1, cpu_list[1].initial_power_limit);

    char core1_filename[BUFSIZ], core2_filename[BUFSIZ];
    sprintf(core1_filename, "/home/cc/penelope_core1_%g.csv", cpu_list[0].initial_power_limit);
    sprintf(core2_filename, "/home/cc/penelope_core2_%g.csv", cpu_list[0].initial_power_limit);

    FILE *core_files[2];
    core_files[0] = fopen(core1_filename, "w");
    core_files[1] = fopen(core2_filename, "w");

    while (!data->ctx->dying)
    {
        // pcm->pcm_c_start();
        sleep(frequency);
        // pcm->pcm_c_stop();

        frequency = DEFAULT_FREQUENCY;
        double nowtime = get_power_reading(power_ctx, cpu_list);

        printf("hello world\n");
        for (int i = 0; i < 2; i++)
        {
            if (cpu_list[i].current_power < 0 || cpu_list[i].current_power > 200)
            {
                break;
            }
            
	        // int lcore_id = i;
            // double instrs = (double)pcm->pcm_c_get_instr(lcore_id);
            // double cycles = (double)pcm->pcm_c_get_cycles(lcore_id);
            // double ipc = instrs / cycles;

	        // printf("C:%f I:%f, IPC:%3.2f\n",
            //     instrs,
            //     cycles,
            //     ipc);

            // if (ipc <= 0) // unsure if I want to do this
            //     break;
            
            // per socket, file with time, ipc, powercap for graphing
            double ipc = -1.0;
            fprintf(core_files[i], "%f,%f,%f,%f\n", 
                    nowtime, 
                    ipc, 
                    cpu_list[i].current_power,
                    cpu_list[i].current_power_limit);

            // insert_node(cpu_list[i].current_power_limit, ipc, ipc_list);

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
                extra_power = 0.0f;
                if (extra_power > 0)
                {
                    cpu_list[i].next_power_limit = cpu_list[i].current_power_limit - extra_power;
                    set_power(i, cpu_list[i].next_power_limit);
                    cpu_list[i].current_power_limit = cpu_list[i].next_power_limit;
                    exempt_from_release = true; // don't need to release more if we've already released
                }

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
                #ifdef VERBOSE
                printf("Under powercap. extra_power: %f\n", extra_power);
                #endif
            } 
            // else if (cpu_list[i].current_power > cpu_list[i].current_power_limit - POWER_MARGIN * 0.5)
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

                    double received_power = 0.0f;
                    for (int j = 0; j < 2; j++)
                    {
                        pthread_mutex_lock(&cpu_list[j].lock);
                        double max_size = max_transaction_amount(cpu_list[j].available_power, 0);
                        double delta_power = (cpu_list[j].available_power < max_size) ? cpu_list[j].available_power : max_size;
                        delta_power = 0.0f;
                        received_power += delta_power;
                        cpu_list[j].available_power -= delta_power;
                        pthread_mutex_unlock(&cpu_list[j].lock);
                    }

                    if (received_power == 0)
                    {
                        // make request
                        power_exchange_msg_t *response = send_power_request(data, sender, false, 0);
                        received_power = (response != NULL) ? response->power_exchanged : 0;
                    }

                    if (received_power > 0)
                    {
                        cpu_list[i].next_power_limit = cpu_list[i].current_power_limit + received_power;
                        set_power(i, cpu_list[i].next_power_limit);
                        cpu_list[i].current_power_limit = cpu_list[i].next_power_limit;
                    }
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

                    double received_power = 0.0f;
                    for (int j = 0; j < 2; j++)
                    {
                        pthread_mutex_lock(&cpu_list[j].lock);
                        double max_size = max_transaction_amount(cpu_list[j].available_power, necessary_power);
                        double delta_power = (cpu_list[j].available_power < max_size) ? cpu_list[j].available_power : max_size;
                        delta_power = 0.0f;
                        received_power += delta_power;
                        cpu_list[j].available_power -= delta_power;
                        pthread_mutex_unlock(&cpu_list[j].lock);
                    }

                    if (received_power == 0)
                    {
                        // make request
                        power_exchange_msg_t *response = send_power_request(data, sender, true, necessary_power);
                        received_power = (response != NULL) ? response->power_exchanged : 0;
                    }

                    if (received_power > 0)
                    {
                        cpu_list[i].next_power_limit = cpu_list[i].current_power_limit + received_power;
                        set_power(i, cpu_list[i].next_power_limit);
                        cpu_list[i].current_power_limit = cpu_list[i].next_power_limit;
                    }

                    if (received_power < necessary_power)
                    {
                        frequency = HALF_FREQUENCY;
                        exempt_from_release = true; // still urgently need power. our urgency overrides
                    }
                    #ifdef VERBOSE
                    printf("Need power, under initial cap. received: %f, needed: %f, new limit: %f\n", 
                            received_power, 
                            necessary_power,
                            cpu_list[i].current_power_limit
                            );
                    #endif
                }
            }
            // else
            // {
            //     // basically do nothing; stable state. will yield to urgency, nothing else.
            //     #ifdef VERBOSE
            //     printf("stable state\n");
            //     #endif
            //     pthread_mutex_lock(&cpu_list[i].lock);
            //     cpu_list[i].urgency = false;
            //     cpu_list[i].class = 2;
            //     pthread_mutex_unlock(&cpu_list[i].lock);
            // }

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

                // #ifdef VERBOSE
                // printf("releasing power\n");
                // #endif
                // power_ipc_node_t *lower_level = lookup(cpu_list[i].current_power_limit, ipc_list);
                // if (lower_level != NULL)
                // {
                //     #ifdef VERBOSE
                //     printf("lookup succeeded\n");
                //     #endif
                //     double percent_diff = (ipc - lower_level->ipc) / ipc; 
                //     if (percent_diff <= 0.05)
                //     {
                //         double extra_power = cpu_list[i].current_power_limit - lower_level->powercap;
                //         cpu_list[i].current_power_limit = cpu_list[i].current_power_limit - extra_power;
                //         set_power(i, cpu_list[i].current_power_limit);
                //         cpu_list[i].available_power += extra_power;
                //         cpu_list[i].release_power = false;
                //     }
                //     #ifdef VERBOSE
                //     printf("power: %f, ipc: %f\n", lower_level->powercap, lower_level->ipc);
                //     #endif
                // }
            }
            pthread_mutex_unlock(&cpu_list[i].lock);
        }
    }
    fflush(core_files[0]);
    fflush(core_files[1]);
    fflush(stdout);
    return NULL;
}
