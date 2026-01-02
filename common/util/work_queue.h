// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _WORK_QUEUE_H_
#define _WORK_QUEUE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "queue.h"

typedef int (*work_func_t)(void *para);

typedef struct
{
	int count;
	int stack_size;
	pthread_t *pthread;
	queue_t queue;
} work_queue_t;

typedef struct
{
	work_func_t work_func;
	void *para;
	struct list_head node;
} work_t;

int work_queue_init(work_queue_t *work_queue, int count, int stack_size);

int work_queue_exit(work_queue_t *work_queue);

work_queue_t *work_queue_create(int count, int stack_size);

int work_queue_destroy(work_queue_t *work_queue);

int work_schedule(work_queue_t *work_queue, work_t *work);

#ifdef __cplusplus
}
#endif

#endif
