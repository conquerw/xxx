/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "nng/nng.h"
#include "nng/protocol/pubsub0/pub.h"
#include "nng/protocol/pubsub0/sub.h"

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
static nng_socket sub_sock;

static double get_microsecond(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);

	return (double)time.tv_sec * 1000 + (double)time.tv_nsec / 1000000;
}

static void *nng_thread(void *para)
{
	while(1)
	{
        char *buffer = NULL;
        size_t buffer_len = 0;
		nng_recv(sub_sock, &buffer, &buffer_len, NNG_FLAG_ALLOC);

        if(!buffer)
            continue;

		if(buffer[0] == 1)
			is_start = 1;
		else if(buffer[0] == 4)
		{
			time_result[i].recv_total_len = *((unsigned int *)(buffer + 1));
			time_result[i].recv_count = *((unsigned int *)(buffer + 5));
			time_result[i].recv_total_time = *((float *)(buffer + 9));

			pthread_mutex_lock(&mutex);
			start_flag = 1;
			pthread_mutex_unlock(&mutex);
			pthread_cond_signal(&cond);
		}

		nng_free(buffer, buffer_len);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
    nng_socket pub_sock;
    int ret = nng_pub0_open(&pub_sock);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
        return -1;
    }

	// ret = nng_listen(pub_sock, "ipc:///tmp/5555", NULL, 0);
	ret = nng_listen(pub_sock, "tcp://127.0.0.1:5555", NULL, 0);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
		nng_close(pub_sock);
		return -1;
    }

	// unsigned int max_msg_size = 102400 * 10240;
	// nng_setopt(pub_sock, NNG_OPT_SENDBUF, &max_msg_size, sizeof(max_msg_size));

    ret = nng_sub0_open(&sub_sock);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
		return -1;
	}

	ret = nng_setopt(sub_sock, NNG_OPT_SUB_SUBSCRIBE, "", 0);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
		return -1;
	}

	// unsigned int pub_recv_buf_size = 1024 * 10240;
	// nng_setopt(sub_sock, NNG_OPT_RECVBUF, &pub_recv_buf_size, sizeof(pub_recv_buf_size));

	ret = -1;

	while(ret != 0)
	{
		// ret = nng_dial(sub_sock, "ipc:///tmp/5556", NULL, 0);
		ret = nng_dial(sub_sock, "tcp://127.0.0.1:5556", NULL, 0);
		if(ret != 0)
            usleep(500 * 1000);
		else
		{
            pthread_t pthread;
			pthread_create(&pthread, NULL, nng_thread, NULL);
			sleep(1);
		}
	}

	sleep(3);

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

	while(is_start == 0)
	{
		char cmd = 1;
		nng_send(pub_sock, &cmd, 1, 0);
		sleep(1);
	}

	double start = 0, end = 0;

	for(i = 0; i < 1; i++)
	{
		char *buffer = (char *)calloc(byte[i], sizeof(char));
		if(!buffer)
			return -1;

		buffer[0] = 2;
		nng_send(pub_sock, buffer, byte[i], 0);

		start = get_microsecond();

		for(unsigned int j = 0; j < sample_len; j++)
		// for(unsigned int j = 0; j < 10; j++)
		{
			double batch_start = get_microsecond();

			for(int k = 0; k < burst_len; k++)
			// for(int k = 0; k < 1; k++)
			{
				time_result[i].send_count++;
				buffer[0] = 3;
				*((unsigned int *)(buffer + 1)) = time_result[i].send_count;
				nng_send(pub_sock, buffer, byte[i], 0);
				// usleep(1);
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
		nng_send(pub_sock, buffer, byte[i], 0);

		time_result[i].send_total_time = end - start;

		if(buffer)
			free(buffer);

		pthread_mutex_lock(&mutex);
		while(!start_flag)
			pthread_cond_wait(&cond, &mutex);
		start_flag = 0;
		pthread_mutex_unlock(&mutex);
	}

	for(i = 0; i < 1; i++)
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
