/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gateway.h"
#include "event.h"

static int event_callback(event_thread_t *event_thread, void *para, unsigned short source_id, unsigned int event, char *buffer, int buffer_len)
{
	printf("%d, %d\n", source_id, event);

	return 1;
}

int main(int argc, char *argv[])
{
	gateway_t *gateway = gateway_create(PORT_MPU_TEST_ROUTE, 1);
	gateway_start(gateway);
	
	event_thread_t *event_thread = event_thread_create(PORT_MPU_TEST_APP0, 10);
	event_thread_attach_event(event_thread, 1234, event_callback, NULL);
	event_thread_attach_event(event_thread, 50000, event_callback, NULL);
	event_thread_start(event_thread);
	
	while(1)
	{
		event_send(event_thread, PORT_MPU_TEST2_APP0, 1235, NULL, 0);
		sleep(1);
	}
	
	return 1;
}
