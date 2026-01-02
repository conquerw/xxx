/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include "mosquitto.h"
#include "util.h"

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

static void connect_callback(struct mosquitto *mosq, void *obj, int reason_code)
{
	if(reason_code != 0)
		return;

	char topic[16] = "";
	memset(topic, 0x00, sizeof(topic));
	snprintf(topic, sizeof(topic), "process%d", 22);

	mosquitto_subscribe(mosq, NULL, topic, 0);
}

static void recv_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	if(!msg)
		return;

	char *buffer = (char *)msg->payload;

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
}

int main(int argc, char *argv[])
{
	set_root_caps();
	
    struct sched_param param;
    param.sched_priority = 80;
	sched_setscheduler(0, SCHED_RR, &param);

	mosquitto_lib_init();
	struct mosquitto *mosquitto = mosquitto_new(NULL, true, NULL);

	mosquitto_connect_callback_set(mosquitto, connect_callback);
	mosquitto_message_callback_set(mosquitto, recv_callback);

	mosquitto_connect(mosquitto, "127.0.0.1", 1883, 60);

	mosquitto_loop_start(mosquitto);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

	char topic[16] = "";
	memset(topic, 0x00, sizeof(topic));
	snprintf(topic, sizeof(topic), "process%d", 11);

	while(is_start == 0)
	{
		char cmd = 1;
		mosquitto_publish(mosquitto, NULL, topic, 1, &cmd, 0, false);
		sleep(1);
	}

	double start = 0, end = 0;

	for(i = 0; i < 4; i++)
	{
		char *buffer = (char *)calloc(byte[i], sizeof(char));
		if(!buffer)
			return -1;

		buffer[0] = 2;
		mosquitto_publish(mosquitto, NULL, topic, byte[i], buffer, 0, false);

		start = get_microsecond();

		for(unsigned int j = 0; j < sample_len; j++)
		{
			double batch_start = get_microsecond();

			for(int k = 0; k < burst_len; k++)
			{
				time_result[i].send_count++;
				buffer[0] = 3;
				*((unsigned int *)(buffer + 1)) = time_result[i].send_count;
				mosquitto_publish(mosquitto, NULL, topic, byte[i], buffer, 0, false);
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
		mosquitto_publish(mosquitto, NULL, topic, byte[i], buffer, 0, false);

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
