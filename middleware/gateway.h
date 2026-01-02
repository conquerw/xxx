// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _GATEWAY_H_
#define _GATEWAY_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include <semaphore.h>
#include "port.h"
#include "middleware.h"

typedef struct
{
	unsigned short id;
	char type;
	queue_t queue;
	port_t *port;
	pthread_t port_pthread;
	sem_t port_sem;
	middleware_ops_t *middleware_ops;
	pthread_t middleware_ops_pthread;
	sem_t middleware_ops_sem;
} gateway_t;

int gateway_init(gateway_t *gateway, unsigned short id, char type);

int gateway_exit(gateway_t *gateway);

gateway_t *gateway_create(unsigned short id, char type);

int gateway_destroy(gateway_t *gateway);

int gateway_start(gateway_t *gateway);

int gateway_stop(gateway_t *gateway);

#ifdef __cplusplus
}
#endif

#endif
