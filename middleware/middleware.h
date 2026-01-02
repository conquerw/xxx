// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _MIDDLEWARE_H_
#define _MIDDLEWARE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "sk_buffer.h"

enum
{
	MIDDLEWARE_TYPE_UNKOWN = 0,
	#if defined(MIDDLEWARE_MOSQUITTO)
	MIDDLEWARE_TYPE_MOSQUITTO,
	#endif
	#if defined(MIDDLEWARE_IOX2)
	MIDDLEWARE_TYPE_IOX2,
	#endif
	#if defined(MIDDLEWARE_NNG)
	MIDDLEWARE_TYPE_NNG,
	#endif
	MIDDLEWARE_TYPE_NUM
};

typedef struct middleware_ops middleware_ops_t;

typedef int (*recv_callback_t) (middleware_ops_t *middleware_ops, char *buffer, int buffer_len);

struct middleware_ops
{
	void *p;
	int (*init) (middleware_ops_t *middleware_ops, unsigned short id);
	int (*exit) (middleware_ops_t *middleware_ops);
	int (*send) (middleware_ops_t *middleware_ops, unsigned short id, sk_buffer_t *sk_buffer);
	int (*set_recv_callback) (middleware_ops_t *middleware_ops, recv_callback_t cb);
	int (*start) (middleware_ops_t *middleware_ops);
	int (*stop) (middleware_ops_t *middleware_ops);
};

middleware_ops_t *middleware_create(char type);

int middleware_destroy(middleware_ops_t *middleware_ops, char type);

static inline void middleware_ops_set_p(middleware_ops_t *middleware_ops, void *p)
{
    middleware_ops->p = p;
}

static inline void *middleware_ops_get_p(middleware_ops_t *middleware_ops)
{
    return middleware_ops->p;
}

#ifdef __cplusplus
}
#endif

#endif
