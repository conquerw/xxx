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

static nng_socket pub_sock, sub_sock;

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

        double start = get_microsecond();

        static unsigned int len = 0, count = 0;
        unsigned int len2 = *((unsigned int *)(buffer + 0));
        if(len != len2)
        {
            len = len2;
            count = 0;
        }

        count++;
        *((unsigned int *)(buffer + 8)) = count;

        double end = get_microsecond();
        *((float *)(buffer + 12)) = (float)(end - start);

        nng_send(pub_sock, buffer, buffer_len, 0);

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

	ret = nng_listen(pub_sock, "ipc:///tmp/5556", NULL, 0);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
		nng_close(pub_sock);
		return -1;
    }

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

	ret = -1;

	while(ret != 0)
	{
		ret = nng_dial(sub_sock, "ipc:///tmp/5555", NULL, 0);
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
		sleep(1);

	return 1;
}
