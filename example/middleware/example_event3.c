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
	gateway_t *gateway = gateway_create(PORT_MPU_TEST3_ROUTE, 1);
	gateway_start(gateway);
	
	event_thread_t *event_thread = event_thread_create(PORT_MPU_TEST3_APP0, 10);
	event_thread_start(event_thread);

	event_thread_t *event_thread2 = event_thread_create(PORT_MPU_TEST3_APP1, 10);
	event_thread_attach_event(event_thread2, 50000, event_callback, NULL);
	event_thread_start(event_thread2);
	
	while(1)
	{
		event_broadcast(event_thread, 50000, NULL, 0);
		sleep(1);
	}

	return 1;
}
