// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <pthread.h>
#include "list.h"

#if defined(BASIC_QUEUE)

typedef struct
{
	int max;
	int len;
	pthread_mutex_t mutex;
	pthread_cond_t push_cond;
	pthread_cond_t pop_cond;
	struct list_head node;
} queue_t;

#elif defined(DOUBLE_LIST_QUEUE)

typedef struct
{
	int max;
	int len;
	pthread_mutex_t push_mutex;
	pthread_mutex_t pop_mutex;
	pthread_cond_t push_cond;
	pthread_cond_t pop_cond;
	struct list_head push_node;
	struct list_head pop_node;
} queue_t;

#endif

int queue_init(queue_t *queue, int max);

int queue_exit(queue_t *queue);

queue_t *queue_create(int max);

int queue_destroy(queue_t *queue);

int queue_push(queue_t *queue, struct list_head *node);

struct list_head *queue_pop(queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif
