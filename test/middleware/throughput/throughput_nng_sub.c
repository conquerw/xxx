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

static double start = 0, end = 0;
static unsigned char is_end = 0;
static unsigned int count = 0, total_len = 0;
static nng_socket pub_sock;
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

		if(buffer[0] == 1)
		{
			char cmd = 1;
			nng_send(pub_sock, &cmd, 1, 0);
		}
		if(buffer[0] == 2)
		{
			is_end = count = total_len = 0;
			start = get_microsecond();
		}
		else if(buffer[0] == 3)
		{
			count++;
			total_len += buffer_len;
			
			// unsigned int send_count = *((unsigned int *)(buffer + 1));
			// printf("send_count, total_len, buffer_len: %d-%d-%d\n", send_count, total_len, buffer_len);
		}
		else if(buffer[0] == 4)
		{
			end = get_microsecond();
			is_end = 1;
		}

		nng_free(buffer, buffer_len);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
    int ret = nng_pub0_open(&pub_sock);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
        return -1;
    }

	// ret = nng_listen(pub_sock, "ipc:///tmp/5556", NULL, 0);
	ret = nng_listen(pub_sock, "tcp://127.0.0.1:5556", NULL, 0);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
		nng_close(pub_sock);
		return -1;
    }

	unsigned int sub_max_msg_size = 1024 * 10240;
	nng_setopt(pub_sock, NNG_OPT_SENDBUF, &sub_max_msg_size, sizeof(sub_max_msg_size));

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

	unsigned int recv_buf_size = 102400 * 10240;
	nng_setopt(sub_sock, NNG_OPT_RECVBUF, &recv_buf_size, sizeof(recv_buf_size));

	ret = -1;

	while(ret != 0)
	{
		// ret = nng_dial(sub_sock, "ipc:///tmp/5555", NULL, 0);
		ret = nng_dial(sub_sock, "tcp://127.0.0.1:5555", NULL, 0);
		if(ret != 0)
            usleep(500 * 1000);
		else
		{
            pthread_t pthread;
			pthread_create(&pthread, NULL, nng_thread, NULL);
			sleep(1);
		}
	}

	while(1)
	{
		if(is_end == 1)
		{
			is_end = 0;

			char buffer[16] = "";
			memset(buffer, 0x00, sizeof(buffer));

			*((unsigned char *)(buffer + 0)) = 5;
			*((unsigned int *)(buffer + 1)) = total_len;
			*((unsigned int *)(buffer + 5)) = count;
			*((float *)(buffer + 9)) = (float)(end - start);

			nng_send(pub_sock, buffer, sizeof(buffer), 0);

			printf("total_len, count: %d, %d\n", total_len, count);
		}

		sleep(1);
	}

	return 1;
}
