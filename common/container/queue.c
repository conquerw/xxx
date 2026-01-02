/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdlib.h>
#include "queue.h"

#if defined(BASIC_QUEUE)

int queue_init(queue_t *queue, int max)
{
	if(!queue)
		return -1;
	
	queue->max = max;
    queue->len = 0;
    pthread_mutex_init(&queue->mutex, NULL);
	pthread_cond_init(&queue->push_cond, NULL);
	pthread_cond_init(&queue->pop_cond, NULL);
	INIT_LIST_HEAD(&queue->node);
	
    return 1;
}

int queue_exit(queue_t *queue)
{
	if(!queue)
		return -1;
	
	INIT_LIST_HEAD(&queue->node);
	pthread_cond_destroy(&queue->pop_cond);
	pthread_cond_destroy(&queue->push_cond);
    pthread_mutex_destroy(&queue->mutex);
	
    return 1;
}

int queue_push(queue_t *queue, struct list_head *node)
{
	if(!queue || !node)
        return -1;

    pthread_mutex_lock(&queue->mutex);
	if(queue->max != -1)
		while(queue->len > queue->max - 1)
			pthread_cond_wait(&queue->push_cond, &queue->mutex);
	queue->len++;
	list_add_tail(node, &queue->node);
	pthread_mutex_unlock(&queue->mutex);
	pthread_cond_signal(&queue->pop_cond);
	
    return 1;
}

struct list_head *queue_pop(queue_t *queue)
{
	if(!queue)
		return NULL;
	
	struct list_head *node = NULL;
    pthread_mutex_lock(&queue->mutex);
	while(queue->len == 0)
		pthread_cond_wait(&queue->pop_cond, &queue->mutex);
	queue->len--;
	node = queue->node.next;
	list_del(node);
	pthread_mutex_unlock(&queue->mutex);
	if(queue->max != -1)
		pthread_cond_signal(&queue->push_cond);
		
	return node;
}

#elif defined(DOUBLE_LIST_QUEUE)

int queue_init(queue_t *queue, int max)
{
	if(!queue)
		return -1;
	
	queue->max = max;
	queue->len = 0;
    pthread_mutex_init(&queue->push_mutex, NULL);
	pthread_mutex_init(&queue->pop_mutex, NULL);
	pthread_cond_init(&queue->push_cond, NULL);
	pthread_cond_init(&queue->pop_cond, NULL);
	INIT_LIST_HEAD(&queue->push_node);
	INIT_LIST_HEAD(&queue->pop_node);
	
    return 1;
}

int queue_exit(queue_t *queue)
{
	if(!queue)
		return -1;
	
	INIT_LIST_HEAD(&queue->pop_node);
	INIT_LIST_HEAD(&queue->push_node);
	pthread_cond_destroy(&queue->pop_cond);
	pthread_cond_destroy(&queue->push_cond);
    pthread_mutex_destroy(&queue->pop_mutex);
	pthread_mutex_destroy(&queue->push_mutex);
	free(queue);
	
    return 1;
}

int queue_push(queue_t *queue, struct list_head *node)
{
	if(!queue || !node)
        return -1;
	
    pthread_mutex_lock(&queue->push_mutex);
	if(queue->max != -1)
		while(queue->len > queue->max - 1)
			pthread_cond_wait(&queue->push_cond, &queue->push_mutex);
	queue->len++;
	list_add_tail(node, &queue->push_node);
	pthread_mutex_unlock(&queue->push_mutex);
	pthread_cond_signal(&queue->pop_cond);

    return 1;
}

static int swap_list(queue_t *queue)
{
	if(!queue)
		return -1;
	
	pthread_mutex_lock(&queue->push_mutex);
	while(queue->len == 0)
		pthread_cond_wait(&queue->pop_cond, &queue->push_mutex);

	int len = queue->len;
	if(queue->max != -1)
		if(len > queue->max - 1)
			pthread_cond_broadcast(&queue->push_cond);

	list_add_tail(&queue->pop_node, &queue->push_node);
	list_del_init(&queue->push_node);
	queue->len = 0;
	pthread_mutex_unlock(&queue->push_mutex);
	
	return len;
}

struct list_head *queue_pop(queue_t *queue)
{
	if(!queue)
		return NULL;
	
	struct list_head *node = NULL;
    pthread_mutex_lock(&queue->pop_mutex);
	if(!list_empty(&queue->pop_node) || swap_list(queue) > 0)
	{
		node = queue->pop_node.next;
		list_del(node);
	}
	pthread_mutex_unlock(&queue->pop_mutex);

	return node;
}

#endif

queue_t *queue_create(int max)
{
	queue_t *queue = (queue_t *)calloc(1, sizeof(queue_t));
	if(!queue)
		return NULL;
	
	int ret = queue_init(queue, max);
	if(ret != 1)
	{
		free(queue);
		return NULL;
	}
	
	return queue;
}

int queue_destroy(queue_t *queue)
{
	if(!queue)
		return -1;
	
	queue_exit(queue);
	free(queue);
	
	return 1;
}
