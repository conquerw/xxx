/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include "util.h"
#include "gateway.h"
#include "event.h"

static double start = 0, end = 0;
static unsigned char is_end = 0;
static unsigned int count = 0, total_len = 0;

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
	{
		char cmd = 1;
		event_send(event_thread, PORT_MPU_TEST_APP0, 1234, &cmd, 1);
	}
	else if(buffer[0] == 2)
	{
		is_end = count = total_len = 0;
		start = get_microsecond();
	}
	else if(buffer[0] == 3)
	{
		count++;
		total_len += buffer_len;
	}
	else if(buffer[0] == 4)
	{
		end = get_microsecond();
		is_end = 1;
	}

	return 1;
}

int main(int argc, char *argv[])
{
	set_root_caps();
	
    struct sched_param param;
    param.sched_priority = 80;
	sched_setscheduler(0, SCHED_RR, &param);

	gateway_t *gateway = gateway_create(PORT_MPU_TEST2_ROUTE, 1);
	gateway_start(gateway);
	
	event_thread_t *event_thread = event_thread_create(PORT_MPU_TEST2_APP0, 10);
	event_thread_attach_event(event_thread, 1235, event_callback, NULL);
	event_thread_start(event_thread);
	
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
			event_send(event_thread, PORT_MPU_TEST_APP0, 1234, buffer, sizeof(buffer));

			printf("total_len, count: %d, %d\n", total_len, count);
		}

		sleep(1);
	}

	return 1;
}
