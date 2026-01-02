// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _MIDDLEWARE_NNG_H_
#define _MIDDLEWARE_NNG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "nng/nng.h"
#include "list.h"
#include "middleware.h"
#include "sk_buffer.h"

typedef struct
{
	unsigned short id;
	nng_socket sock;
} nng_t;

typedef struct
{
	nng_t nng;
	struct list_head node;
	pthread_mutex_t node_mutex;
	middleware_ops_t middleware_ops;
} middleware_nng_t;

int middleware_nng_init(middleware_ops_t *middleware_ops, unsigned short id);

int middleware_nng_exit(middleware_ops_t *middleware_ops);

int middleware_nng_send(middleware_ops_t *middleware_ops, unsigned short id, sk_buffer_t *sk_buffer);

int middleware_nng_set_recv_callback(middleware_ops_t *middleware_ops, recv_callback_t cb);

int middleware_nng_start(middleware_ops_t *middleware_ops);

int middleware_nng_stop(middleware_ops_t *middleware_ops);

#ifdef __cplusplus
}
#endif

#endif
