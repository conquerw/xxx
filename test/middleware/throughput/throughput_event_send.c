/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <sched.h>
#include "list.h"
#include "util.h"
#include "gateway.h"
#include "event.h"

// test protocol
// send: cmd + send_count
// recv: cmd + recv_total_len + recv_count + recv_total_time

#define BYTE_LEN 6

typedef struct
{
	unsigned int send_total_len;
	unsigned int send_count;
	float send_total_time;
	unsigned int recv_total_len;
	unsigned int recv_count;
	float recv_total_time;
} time_result_t;

static unsigned int byte[BYTE_LEN] = {64, 256, 1024, 4096, 16384, 65536};
static unsigned int sample_len = 1000, burst_len = 500, recovery_time_us = 1000;
static time_result_t time_result[BYTE_LEN];
static unsigned char is_start = 0;
static int i = 0;
static pthread_mutex_t mutex;
static pthread_cond_t cond;
static int start_flag = 0;

static double get_microsecond(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);

	return (double)time.tv_sec * 1000 + (double)time.tv_nsec / 1000000;
}

static int event_callback(event_thread_t *event_thread, void *para, unsigned short source_id, unsigned int event, char *buffer, int buffer_len)
{
	if(!buffer)
		return -1;

	if(buffer[0] == 1)
		is_start = 1;
	else if(buffer[0] == 5)
	{
		time_result[i].recv_total_len = *((unsigned int *)(buffer + 1));
		time_result[i].recv_count = *((unsigned int *)(buffer + 5));
		time_result[i].recv_total_time = *((float *)(buffer + 9));

		pthread_mutex_lock(&mutex);
		start_flag = 1;
		pthread_mutex_unlock(&mutex);
		pthread_cond_signal(&cond);
	}

	return 1;
}

int main(int argc, char *argv[])
{
	set_root_caps();
	
    struct sched_param param;
    param.sched_priority = 80;
	sched_setscheduler(0, SCHED_RR, &param);

	gateway_t *gateway = gateway_create(PORT_MPU_TEST_ROUTE, 1);
	gateway_start(gateway);
	
	event_thread_t *event_thread = event_thread_create(PORT_MPU_TEST_APP0, 10);
	event_thread_attach_event(event_thread, 1234, event_callback, NULL);
	event_thread_start(event_thread);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

	while(is_start == 0)
	{
		char cmd = 1;
		event_send(event_thread, PORT_MPU_TEST2_APP0, 1235, &cmd, 1);
		sleep(1);
	}

	double start = 0, end = 0;

	for(i = 0; i < 4; i++)
	{
		char *buffer = (char *)calloc(byte[i], sizeof(char));
		if(!buffer)
			return -1;

		buffer[0] = 2;
		event_send(event_thread, PORT_MPU_TEST2_APP0, 1235, buffer, byte[i]);
		
		start = get_microsecond();

		for(unsigned int j = 0; j < sample_len; j++)
		{
			double batch_start = get_microsecond();

			for(int k = 0; k < burst_len; k++)
			{
				time_result[i].send_count++;
				buffer[0] = 3;
				*((unsigned int *)(buffer + 1)) = time_result[i].send_count;
				event_send(event_thread, PORT_MPU_TEST2_APP0, 1235, buffer, byte[i]);
			}

			end = get_microsecond();

            // If the batch took less than the recovery time, sleep for the difference recovery_time_us - batch_duration.
            // Else, go ahead with the next batch without time to recover.
			unsigned int time_difference = (end - batch_start) * 1000;
			if(time_difference < recovery_time_us)
				usleep(recovery_time_us - time_difference);
		}

		end = get_microsecond();

		buffer[0] = 4;
		event_send(event_thread, PORT_MPU_TEST2_APP0, 1235, buffer, byte[i]);

		time_result[i].send_total_time = end - start;

		if(buffer)
			free(buffer);

		pthread_mutex_lock(&mutex);
		while(!start_flag)
			pthread_cond_wait(&cond, &mutex);
		start_flag = 0;
		pthread_mutex_unlock(&mutex);
	}

	for(i = 0; i < 4; i++)
	{
		time_result[i].send_total_len = byte[i] * sample_len * burst_len;
		time_result[i].send_total_time /= 1000;
		time_result[i].recv_total_time /= 1000;

		printf("%d, %d\n", byte[i], time_result[i].send_total_len);
		printf("%d, %.2fs, %.2fMBits/s\n", time_result[i].send_count, time_result[i].send_total_time, time_result[i].send_total_len / time_result[i].send_total_time / 1048576);
		printf("%d, %.2fs, %.2fMBits/s\n", time_result[i].recv_count, time_result[i].recv_total_time, time_result[i].recv_total_len / time_result[i].recv_total_time / 1048576);
	}

	return 1;
}
