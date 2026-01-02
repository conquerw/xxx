// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _MIDDLEWARE_MOSQUITTO_H_
#define _MIDDLEWARE_MOSQUITTO_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "mosquitto.h"
#include "middleware.h"
#include "sk_buffer.h"

typedef struct
{
	char id[16];
	struct mosquitto *mosquitto;
	recv_callback_t cb;
} mosquitto_t;

typedef struct
{
	mosquitto_t mosquitto;
	middleware_ops_t middleware_ops;
} middleware_mosquitto_t;

int middleware_mosquitto_init(middleware_ops_t *middleware_ops, unsigned short id);

int middleware_mosquitto_exit(middleware_ops_t *middleware_ops);

int middleware_mosquitto_send(middleware_ops_t *middleware_ops, unsigned short id, sk_buffer_t *sk_buffer);

int middleware_mosquitto_set_recv_callback(middleware_ops_t *middleware_ops, recv_callback_t cb);

int middleware_mosquitto_start(middleware_ops_t *middleware_ops);

int middleware_mosquitto_stop(middleware_ops_t *middleware_ops);

#ifdef __cplusplus
}
#endif

#endif
