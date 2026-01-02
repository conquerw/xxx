// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _MIDDLEWARE_IOX2_H_
#define _MIDDLEWARE_IOX2_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <semaphore.h>
#include "iox2/iceoryx2.h"
#include "middleware.h"
#include "sk_buffer.h"

typedef struct middleware_iox2 middleware_iox2_t;

typedef struct
{
	char id[16];
	iox2_service_name_h name;
	iox2_port_factory_pub_sub_h service;
	iox2_subscriber_h subscriber;
	iox2_port_factory_event_h event_service;
	iox2_listener_h listener;
	pthread_t pthread;
	sem_t sem;
	middleware_iox2_t *middleware_iox2;
} iox2_sub_t;

struct middleware_iox2
{
	iox2_node_h node_handle;
	iox2_sub_t iox2_sub[2];
	recv_callback_t cb;
	pthread_mutex_t cb_mutex;
	struct list_head node;
	pthread_mutex_t node_mutex;
	middleware_ops_t middleware_ops;
};

int middleware_iox2_init(middleware_ops_t *middleware_ops, unsigned short id);

int middleware_iox2_exit(middleware_ops_t *middleware_ops);

int middleware_iox2_send(middleware_ops_t *middleware_ops, unsigned short id, sk_buffer_t *sk_buffer);

int middleware_iox2_set_recv_callback(middleware_ops_t *middleware_ops, recv_callback_t cb);

int middleware_iox2_start(middleware_ops_t *middleware_ops);

int middleware_iox2_stop(middleware_ops_t *middleware_ops);

#ifdef __cplusplus
}
#endif

#endif
