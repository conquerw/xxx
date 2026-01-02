// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _EVENT_H_
#define _EVENT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <semaphore.h>
#include "port.h"

typedef struct event_thread event_thread_t;

typedef int (*event_callback_t)(event_thread_t *event_thread, void *para, unsigned short source_id, unsigned int event, char *buffer, int buffer_len);

typedef struct
{
	unsigned int event;
	event_callback_t cb;
	void *para;
	struct list_head node;
} event_t;

struct event_thread
{
	struct list_head node;
	unsigned short id;
	port_t *port;
	pthread_t pthread;
	sem_t port_sem;
	struct list_head event_node;
	pthread_mutex_t event_node_mutex;
};

int event_thread_init(event_thread_t *event_thread, unsigned short id, unsigned char priority);

int event_thread_exit(event_thread_t *event_thread);

event_thread_t *event_thread_create(unsigned short id, unsigned char priority);

int event_thread_destroy(event_thread_t *event_thread);

int event_thread_attach_event(event_thread_t *event_thread, unsigned int event, event_callback_t cb, void *para);

int event_thread_detach_event(event_thread_t *event_thread, unsigned int event);

int event_thread_start(event_thread_t *event_thread);

int event_thread_stop(event_thread_t *event_thread);

int event_send(event_thread_t *event_thread, unsigned short dest, unsigned int event, char *buffer, int buffer_len);

int event_broadcast(event_thread_t *event_thread, unsigned int event, char *buffer, int buffer_len);

#ifdef __cplusplus
}
#endif

#endif
