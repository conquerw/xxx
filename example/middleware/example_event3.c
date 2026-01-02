/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gateway.h"
#include "event.h"

int main(int argc, char *argv[])
{
	gateway_t *gateway = gateway_create(PORT_MPU_TEST3_ROUTE, 1);
	gateway_start(gateway);
	
	event_thread_t *event_thread = event_thread_create(PORT_MPU_TEST3_APP0, 10);
	event_thread_start(event_thread);
	
	while(1)
	{
		event_broadcast(event_thread, 50000, NULL, 0);
		sleep(1);
	}

	return 1;
}
