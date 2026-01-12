/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include "xxhash.h"
#include "util.h"
#include "sk_buffer.h"
#include "event.h"

static char is_first = 0;
static struct list_head event_thread_list;
static pthread_mutex_t event_thread_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static event_thread_t *event_thread_get_by_id(unsigned short id)
{
	event_thread_t *ret = NULL;
	
	if(id == PORT_UNKOWN || id == PORT_BROADCAST)
		return NULL;
	
	pthread_mutex_lock(&event_thread_list_mutex);
	
	event_thread_t *link = NULL;
	list_for_each_entry(link, &event_thread_list, node)
	{
		if(link->id == id)
		{
			ret = link;
			goto exit;
		}
	}
	
exit:
	pthread_mutex_unlock(&event_thread_list_mutex);
	
	return ret;
}

static int event_thread_broadcast_callback(port_t *port, sk_buffer_t *sk_buffer)
{
	if(!port || !sk_buffer)
		return -1;
	
	event_thread_t *event_thread = (event_thread_t *)port_get_p(port);
	if(!event_thread)
		return -1;
	
	int len = sk_buffer->tail - sk_buffer->data;
	sk_buffer_t *sk_buffer2 = sk_buffer_create(0, len, 0);
	if(!sk_buffer2)
		return -1;

	sk_buffer_data_copy(sk_buffer2, sk_buffer->data, len);
	queue_push(&port->queue, &sk_buffer2->node);

	return 1;
}

int event_thread_init(event_thread_t *event_thread, unsigned short id, unsigned char priority)
{
	if(!event_thread || id == PORT_UNKOWN || id == PORT_BROADCAST)
		return -1;
	
	pthread_mutex_lock(&event_thread_list_mutex);
	
	if(is_first == 0)
	{
		is_first = 1;
		INIT_LIST_HEAD(&event_thread_list);
	}
	
	pthread_mutex_unlock(&event_thread_list_mutex);
	
	event_thread_t *event_thread2 = event_thread_get_by_id(id);
	if(event_thread2)
		return -1;
	
	event_thread->id = id;
	event_thread->port = port_create(event_thread->id);
	if(!event_thread->port)
		return -1;

	port_set_broadcast_callback(event_thread->port, event_thread_broadcast_callback);
	port_set_p(event_thread->port, event_thread);

	sem_init(&event_thread->port_sem, 0, 0);

	INIT_LIST_HEAD(&event_thread->event_node);
	pthread_mutex_init(&event_thread->event_node_mutex, NULL);
	
	pthread_mutex_lock(&event_thread_list_mutex);
	list_add_tail(&event_thread->node, &event_thread_list);
	pthread_mutex_unlock(&event_thread_list_mutex);
	
	return 1;
}

int event_thread_exit(event_thread_t *event_thread)
{
	int ret = -1;
	
	if(!event_thread)
		return ret;
	
	pthread_mutex_lock(&event_thread_list_mutex);
	
	event_thread_t *link = NULL, *next = NULL;
	list_for_each_entry_safe(link, next, &event_thread_list, node)
	{
		if(link == event_thread)
		{
			list_del(&link->node);
			sem_destroy(&link->port_sem);
			port_destroy(link->port);
			ret = 1;
			goto exit;
		}
    }
	
exit:
	pthread_mutex_unlock(&event_thread_list_mutex);
	
	return ret;
}

event_thread_t *event_thread_create(unsigned short id, unsigned char priority)
{
	if(id == PORT_UNKOWN || id == PORT_BROADCAST)
		return NULL;
	
	event_thread_t *event_thread = (event_thread_t *)calloc(1, sizeof(event_thread_t));
	if(!event_thread)
		return NULL;
	
	int ret = event_thread_init(event_thread, id, priority);
	if(ret != 1)
	{
		free(event_thread);
		return NULL;
	}
	
	return event_thread;
}

int event_thread_destroy(event_thread_t *event_thread)
{
	if(!event_thread)
		return -1;
	
	event_thread_exit(event_thread);
	free(event_thread);
	
	return 1;
}

static event_t *event_get_by_id(event_thread_t *event_thread, unsigned int event)
{
	event_t *ret = NULL;
	
	if(!event_thread)
		return NULL;
	
	pthread_mutex_lock(&event_thread->event_node_mutex);
	
	event_t *link = NULL;
	list_for_each_entry(link, &event_thread->event_node, node)
	{
		if(link->event == event)
		{
			ret = link;
			goto exit;
		}
	}
	
exit:
	pthread_mutex_unlock(&event_thread->event_node_mutex);
	
	return ret;
}

int event_thread_attach_event(event_thread_t *event_thread, unsigned int event, event_callback_t cb, void *para)
{
	if(!event_thread || !cb)
		return -1;
	
	event_t *event2 = event_get_by_id(event_thread, event);
	if(event2)
		return 1;
	
	event2 = (event_t *)calloc(1, sizeof(event_t));
	if(!event2)
		return -1;
	
	event2->event = event;
	event2->cb = cb;
	event2->para = para;
	
	pthread_mutex_lock(&event_thread->event_node_mutex);
	list_add_tail(&event2->node, &event_thread->event_node);
	pthread_mutex_unlock(&event_thread->event_node_mutex);
	
	return 1;
}

int event_thread_detach_event(event_thread_t *event_thread, unsigned int event)
{
	int ret = -1;
	
	if(!event_thread)
		return -1;
	
	pthread_mutex_lock(&event_thread->event_node_mutex);
	
	event_t *link = NULL, *next = NULL;
	list_for_each_entry_safe(link, next, &event_thread->event_node, node)
	{
		if(link->event == event)
		{
			list_del(&link->node);
			free(link);
			ret = 1;
			goto exit;
		}
    }
	
exit:
	pthread_mutex_unlock(&event_thread->event_node_mutex);
	
	return ret;
}

static void *event_thread2(void *para)
{
	if(!para)
		return NULL;
	
	event_thread_t *event_thread = (event_thread_t *)para;
	if(!event_thread->port)
		return NULL;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	port_set_state(event_thread->port, PORT_STATE_CONN);

	sem_post(&event_thread->port_sem);

	while(1)
	{
		sk_buffer_t *sk_buffer = port_recv(event_thread->port);
		if(!sk_buffer)
			continue;

		unsigned int hash = *(unsigned int *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 4);
		XXH32_hash_t hash2 = XXH32(sk_buffer->data, sk_buffer->tail - sk_buffer->data, 0);
		if(hash != hash2)
		{
			printf("hash values are not equal, hash-hash2: %x-%x\n", hash, hash2);
			sk_buffer_destroy(sk_buffer);
			continue;
		}

		unsigned short source = *(unsigned short *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 2);
		sk_buffer_pull(sk_buffer, 2);
		unsigned int event = *(unsigned int *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 4);
		unsigned int len = *(unsigned int *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 4);

		pthread_mutex_lock(&event_thread->event_node_mutex);
		
		event_t *link = NULL;
		list_for_each_entry(link, &event_thread->event_node, node)
		{
			if(link->event == event)
			{
				if(link->cb)
					link->cb(event_thread, link->para, source, event, sk_buffer->data, len);
			}
		}
		
		pthread_mutex_unlock(&event_thread->event_node_mutex);

		sk_buffer_destroy(sk_buffer);
	}
	
	return NULL;
}

int event_thread_start(event_thread_t *event_thread)
{
	if(!event_thread)
		return -1;

	set_root_caps();

    pthread_attr_t attr;
    struct sched_param param;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	param.sched_priority = 80;
	pthread_attr_setschedparam(&attr, &param);

	int ret = pthread_create(&event_thread->pthread, &attr, event_thread2, event_thread);
	if(ret != 0)
		printf("pthread_create failed: %s\n", strerror(ret));
	pthread_detach(event_thread->pthread);
	sem_wait(&event_thread->port_sem);

	pthread_attr_destroy(&attr);

	return 1;
}

int event_thread_stop(event_thread_t *event_thread)
{
	if(!event_thread)
		return -1;

	port_set_state(event_thread->port, PORT_STATE_DISCONN);
	pthread_cancel(event_thread->pthread);

	return 1;
}

int event_send(event_thread_t *event_thread, unsigned short dest, unsigned int event, char *buffer, int buffer_len)
{
	if(!event_thread || dest == PORT_UNKOWN)
		return -1;

	sk_buffer_t *sk_buffer = sk_buffer_create(HEADROOM_SIZE, buffer_len, TAILROOM_SIZE);
	if(!sk_buffer)
		return -1;

	sk_buffer_data_copy(sk_buffer, buffer, buffer_len);
    sk_buffer_push_copy(sk_buffer, (char *)&buffer_len, 4);
	sk_buffer_push_copy(sk_buffer, (char *)&event, 4);
	sk_buffer_push_copy(sk_buffer, (char *)&dest, 2);
	sk_buffer_push_copy(sk_buffer, (char *)&event_thread->id, 2);
	XXH32_hash_t hash = XXH32(sk_buffer->data, sk_buffer->tail - sk_buffer->data, 0);
	sk_buffer_push_copy(sk_buffer, (char *)&hash, 4);

	int ret = port_send(event_thread->port, dest, sk_buffer);
	if(ret == -1)
		sk_buffer_destroy(sk_buffer);
		
	return 1;
}

int event_broadcast(event_thread_t *event_thread, unsigned int event, char *buffer, int buffer_len)
{
	if(!event_thread)
		return -1;

	sk_buffer_t *sk_buffer = sk_buffer_create(HEADROOM_SIZE, buffer_len, TAILROOM_SIZE);
	if(!sk_buffer)
		return -1;

	sk_buffer_data_copy(sk_buffer, buffer, buffer_len);
    sk_buffer_push_copy(sk_buffer, (char *)&buffer_len, 4);
	sk_buffer_push_copy(sk_buffer, (char *)&event, 4);
	unsigned short dest = PORT_BROADCAST;
	sk_buffer_push_copy(sk_buffer, (char *)&dest, 2);
	sk_buffer_push_copy(sk_buffer, (char *)&event_thread->id, 2);
	XXH32_hash_t hash = XXH32(sk_buffer->data, sk_buffer->tail - sk_buffer->data, 0);
	sk_buffer_push_copy(sk_buffer, (char *)&hash, 4);
	
	port_broadcast(event_thread->port, sk_buffer);
	sk_buffer_destroy(sk_buffer);

	return 1;
}
