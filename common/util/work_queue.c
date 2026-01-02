/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "work_queue.h"

static void *work_queue_thread(void *para)
{
	if(!para)
		return NULL;
	
	work_queue_t *work_queue = (work_queue_t *)para;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	while(1)
	{
		struct list_head *node = queue_pop(&work_queue->queue);
		if(!node)
			continue;
		
		work_t *work = container_of(node, work_t, node);
		if(!work)
			continue;
		
		if(work->work_func)
			work->work_func(work->para);
		free(work);
	}
	
	return NULL;
}

int work_queue_init(work_queue_t *work_queue, int count, int stack_size)
{
	if(!work_queue || count == 0)
		return -1;
	
	work_queue->count = count;
	work_queue->stack_size = stack_size;
	work_queue->pthread = (pthread_t *)calloc(work_queue->count, sizeof(pthread_t));
	if(!work_queue->pthread)
		return -1;
	
	queue_init(&work_queue->queue, -1);
	
	pthread_attr_t pthread_attr;
	pthread_attr_init(&pthread_attr);
	
	if(work_queue->stack_size)
		pthread_attr_setstacksize(&pthread_attr, work_queue->stack_size);
	
	for(int i = 0; i < work_queue->count; i++)
	{
		pthread_create(work_queue->pthread + i, &pthread_attr, work_queue_thread, work_queue);
		pthread_detach(*(work_queue->pthread + i));
	}
	
	pthread_attr_destroy(&pthread_attr);
	
	return 1;
}

int work_queue_exit(work_queue_t *work_queue)
{
	if(!work_queue)
		return -1;
	
	for(int i = 0; i < work_queue->count; i++)
		pthread_cancel(*(work_queue->pthread + i));
	
	queue_exit(&work_queue->queue);
	
	if(work_queue->pthread)
		free(work_queue->pthread);
	
	return 1;
}

work_queue_t *work_queue_create(int count, int stack_size)
{
	if(count == 0)
		return NULL;
	
	work_queue_t *work_queue = (work_queue_t *)calloc(1, sizeof(work_queue_t));
	if(!work_queue)
		return NULL;
	
	int ret = work_queue_init(work_queue, count, stack_size);
	if(ret != 1)
	{
		free(work_queue);
		return NULL;
	}
	
	return work_queue;
}

int work_queue_destroy(work_queue_t *work_queue)
{
	if(!work_queue)
		return -1;
	
	work_queue_exit(work_queue);
	free(work_queue);
	
	return 1;
}

int work_schedule(work_queue_t *work_queue, work_t *work)
{
	if(!work_queue || !work)
		return -1;
	
	queue_push(&work_queue->queue, &work->node);
	
	return 1;
}
