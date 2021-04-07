/**
 * Contains all code needed for the local decider's operation
 */

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
#define POWER_MARGIN 3 // margin used to determine if a node is power-hungry or has excess
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

// computes how much power we can draw from a pool with the given size
// if necessary power is non-zero, we've received an urgent request and we give
// out however much power is necessary. Power pool makes sure we don't overdraw
// else we return 10% of the pool_size, max of 30, min of 1
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

// Atomically gets power readings from circular buffer defined in rapl.h/c
// Avergaes over last 2 seconds of power readings
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

// Syscalls to RAPL executable to set poweraps for a given socket
// "power" is a variable indicating the powercap to be set, 
// "id" is the socket (0 or 1)
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

// Compute how much excess we have
double calc_extra_power(double current_power_limit, double current_power)
{
    if (current_power <= MINIMUM_POWER_CAP)
        return (current_power_limit - MINIMUM_POWER_CAP);
    else
        return (current_power_limit - current_power - POWER_MARGIN * 0.5);
}

// Randomly choose a node and query their power pool for power. If urgent,
// set flag and provide non-zero necessary_power. Else necessary_power should be 0
power_exchange_msg_t *send_power_request(client_ctx_t *data, void *socket, bool urgency, double necessary_power)
{
    int rc, n;
    uint32_t index = arc4random_uniform(data->len); // uniform random query
    assert(index < data->len);
    #ifdef VERBOSE
    printf("sending message to %i, %s\n", index, data->hosts[index]);
    #endif
    char *host = data->hosts[index];
    char endpoint[BUFSIZ];
    sprintf(endpoint, "tcp://%s:1600", host);
    rc = zmq_connect(socket, endpoint);
    if (rc == -1)
    {
        #ifdef VERBOSE
        printf("failed to connect\n");
        #endif
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
        rc = zmq_disconnect(socket, endpoint); assert(rc == 0);
        #ifdef VERBOSE
        if (rc2 == -1)
            printf("failed to receive response\n");
        #endif
        return (rc2 == -1) ? NULL : (power_exchange_msg_t *)zmq_msg_data(&rec_msg);
    }
    #ifdef VERBOSE
    printf("failed to send %d, %s\n", n, strerror(errno));
    #endif
    return NULL;
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

    // make sure that initial caps are set
    set_power(0, cpu_list[0].initial_power_limit);
    set_power(1, cpu_list[1].initial_power_limit);

    // Files for each core to print logs to
    char core1_filename[BUFSIZ], core2_filename[BUFSIZ];
    sprintf(core1_filename, "/home/cc/penelope_core1_%g.csv", cpu_list[0].initial_power_limit);
    sprintf(core2_filename, "/home/cc/penelope_core2_%g.csv", cpu_list[0].initial_power_limit);

    FILE *core_files[2];
    core_files[0] = fopen(core1_filename, "w");
    core_files[1] = fopen(core2_filename, "w");

    while (!data->ctx->dying)
    {
        // sleep phase of the control loop
        sleep(frequency);

        frequency = DEFAULT_FREQUENCY;
        double nowtime = get_power_reading(power_ctx, cpu_list);

        printf("hello world\n");
        for (int i = 0; i < 2; i++)
        {
            if (cpu_list[i].current_power < 0 || cpu_list[i].current_power > 200)
            {
                break;
            }

            double ipc = -1.0; // legacy, needed to keep this format of 4 numbers for log analysis functions
            fprintf(core_files[i], "%f,%f,%f,%f\n", 
                    nowtime, 
                    ipc, 
                    cpu_list[i].current_power,
                    cpu_list[i].current_power_limit);

            bool exempt_from_release = false; // flag indicating that it doesn't need to release power bc of an urgent request
            #ifdef VERBOSE
            printf("power: %f, cap: %f\n", 
                    cpu_list[i].current_power,
                    cpu_list[i].current_power_limit);
            #endif
            // Case 1: Excess power
            if (cpu_list[i].current_power < cpu_list[i].current_power_limit - POWER_MARGIN)
            {
                // compute extra power, lower cap first by corresponding amount
                double extra_power = calc_extra_power(cpu_list[i].current_power_limit, cpu_list[i].current_power);
                if (extra_power > 0)
                {
                    cpu_list[i].next_power_limit = cpu_list[i].current_power_limit - extra_power;
                    set_power(i, cpu_list[i].next_power_limit);
                    cpu_list[i].current_power_limit = cpu_list[i].next_power_limit;
                    exempt_from_release = true; // don't need to release more if we've already released
                }

                // acquire lock for socket info, set metadata and add excess to
                // available_power
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
            // Case 2: Power-hungry
            else
            {
                // not necessary; randomly request power, but with no urgency
                // also will yield power to requesting threads
                if (cpu_list[i].current_power_limit >= cpu_list[i].initial_power_limit)
                {
                    pthread_mutex_lock(&cpu_list[i].lock);
                    cpu_list[i].urgency = false;
                    cpu_list[i].class = 3;
                    pthread_mutex_unlock(&cpu_list[i].lock);

                    // First check local pool for power
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

                    // If no excess locally, query an external node
                    if (received_power == 0)
                    {
                        // make request
                        power_exchange_msg_t *response = send_power_request(data, sender, false, 0);
                        received_power = (response != NULL) ? response->power_exchanged : 0;
                    }
                    // If either local or external query returned power, raise
                    // our cap
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
                    
                    // First check locally. If nothing available, send an urgent
                    // request 
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
                        power_exchange_msg_t *response = send_power_request(data, sender, true, necessary_power);
                        received_power = (response != NULL) ? response->power_exchanged : 0;
                    }

                    if (received_power > 0)
                    {
                        cpu_list[i].next_power_limit = cpu_list[i].current_power_limit + received_power;
                        set_power(i, cpu_list[i].next_power_limit);
                        cpu_list[i].current_power_limit = cpu_list[i].next_power_limit;
                    }
                    // If still urgent, we're exempt from releasing power
                    // induced by other urgent requests
                    if (received_power < necessary_power)
                    {
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

            // If the local pool has received an urgent request, the decider is
            // not in a state that is exempt from release, and the node is above
            // its initial cap, it lowers its cap to the initial setting, and
            // adds the excess to its local power pool.
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
    fflush(core_files[0]);
    fflush(core_files[1]);
    fflush(stdout);
    return NULL;
}
