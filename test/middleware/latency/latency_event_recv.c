/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "gateway.h"
#include "event.h"

static double get_microsecond(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);

	return (double)time.tv_sec * 1000 + (double)time.tv_nsec / 1000000;
}

static int event_callback(event_thread_t *event_thread, void *para, unsigned short source_id, unsigned int event, char *buffer, int buffer_len)
{
	if(!event_thread || !buffer)
		return -1;

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

	event_send(event_thread, PORT_MPU_TEST_APP0, 1234, buffer, buffer_len);

	return 1;
}

int main(int argc, char *argv[])
{
	gateway_t *gateway = gateway_create(PORT_MPU_TEST2_ROUTE, 1);
	gateway_start(gateway);
	
	event_thread_t *event_thread = event_thread_create(PORT_MPU_TEST2_APP0, 10);
	event_thread_attach_event(event_thread, 1235, event_callback, NULL);
	event_thread_start(event_thread);

	while(1)
		sleep(1);

	return 1;
}
