/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "util.h"
#include "port.h"

static char is_first = 0;
static struct list_head port_list;
static pthread_mutex_t port_list_mutex = PTHREAD_MUTEX_INITIALIZER;

port_t *port_get_by_id(unsigned short id);

int port_init(port_t *port, unsigned short id)
{
	if(!port || id == PORT_UNKOWN || id == PORT_BROADCAST)
		return -1;
	
	pthread_mutex_lock(&port_list_mutex);
	
	if(is_first == 0)
	{
		is_first = 1;
		INIT_LIST_HEAD(&port_list);
	}
	
	pthread_mutex_unlock(&port_list_mutex);
	
	port_t *port2 = port_get_by_id(id);
	if(port2)
		return -1;
	
	port->id = id;
	port->state = PORT_STATE_INIT;
	queue_init(&port->queue, -1);
	
	pthread_mutex_lock(&port_list_mutex);
	list_add_tail(&port->node, &port_list);
	pthread_mutex_unlock(&port_list_mutex);
	
	return 1;
}

int port_exit(port_t *port)
{
	int ret = -1;
	
	if(!port)
		return ret;
	
	pthread_mutex_lock(&port_list_mutex);
	
	port_t *link = NULL, *next = NULL;
	list_for_each_entry_safe(link, next, &port_list, node)
	{
		if(link == port)
		{
			list_del(&link->node);
			queue_exit(&link->queue);
			link->state = PORT_STATE_EXIT;
			ret = 1;
			goto exit;
		}
    }
	
exit:
	pthread_mutex_unlock(&port_list_mutex);
	
	return ret;
}

port_t *port_create(unsigned short id)
{
	if(id == PORT_UNKOWN || id == PORT_BROADCAST)
		return NULL;
	
	port_t *port = (port_t *)calloc(1, sizeof(port_t));
	if(!port)
		return NULL;
	
	int ret = port_init(port, id);
	if(ret != 1)
	{
		free(port);
		return NULL;
	}
	
	return port;
}

int port_destroy(port_t *port)
{
	if(!port)
		return -1;
	
	port_exit(port);
	free(port);
	
	return 1;
}

port_t *port_get_by_id(unsigned short id)
{
	port_t *ret = NULL;
	
	if(id == PORT_UNKOWN || id == PORT_BROADCAST)
		return NULL;
	
	pthread_mutex_lock(&port_list_mutex);
	
	port_t *link = NULL;
	list_for_each_entry(link, &port_list, node)
	{
		if(link->id == id)
		{
			ret = link;
			goto exit;
		}
	}
	
exit:
	pthread_mutex_unlock(&port_list_mutex);
	
	return ret;
}

unsigned short port_get_next_jump(unsigned short current, unsigned short dest)
{
    unsigned short next = PORT_UNKOWN;

    if(current == PORT_UNKOWN || current == PORT_BROADCAST || dest == PORT_UNKOWN || dest == PORT_BROADCAST)
		return next;

	if(PORT_X(dest) == 0)
	{
        if(PORT_X(current) == 0)
            return dest;
		
        if((PORT_Z(current) != 1) && (PORT_Y(current) != 0))
		{
            next = PORT_ROUTE(current);
            return next;
        }
		
        if(current != PORT_MPU_ROUTE && current != PORT_CHIP_ROUTE)
            return PORT_MPU_ROUTE;
		
        if(current == PORT_MPU_ROUTE)
            return PORT_CHIP_ROUTE;
		
        if(current == PORT_CHIP_ROUTE)
            return PORT_MCU_ROUTE;
		
        return PORT_UNKOWN;
    }
	else if(PORT_X(dest) == 1)
	{
		if(PORT_X(current) == 0)
		{
			if(current != PORT_MCU_ROUTE)
				return PORT_MCU_ROUTE;
			else
				return PORT_CHIP_ROUTE;
		}

		if(PORT_Y(current) == PORT_Y(dest))
			return dest;
		
		if(PORT_Z(current) != 1)
		{
			next = PORT_ROUTE(current);
			return next;
		}
		else
		{
			next = PORT_ROUTE(dest);
			return next;
		}
	}

    return PORT_UNKOWN;
}

int port_send(port_t *port, unsigned short dest, sk_buffer_t *sk_buffer)
{
	if(!port || dest == PORT_UNKOWN || !sk_buffer)
		return -1;

	char state = port_get_state(port);
	if(state != PORT_STATE_CONN)
		return -1;

	port_t *dest_port = port_get_by_id(dest);
	if(!dest_port)
	{
		unsigned short next = port_get_next_jump(port->id, dest);
		if(next != PORT_UNKOWN)
			dest_port = port_get_by_id(next);
	}
	
	state = port_get_state(dest_port);
	if(state != PORT_STATE_CONN)
		return -1;

	queue_push(&dest_port->queue, &sk_buffer->node);
	
	return 1;
}

int port_broadcast(port_t *port, sk_buffer_t *sk_buffer)
{
	if(!port || !sk_buffer)
		return -1;
	
	char state = port_get_state(port);
	if(state != PORT_STATE_CONN)
		return -1;
	
	pthread_mutex_lock(&port_list_mutex);
	
	port_t *link = NULL;
	list_for_each_entry(link, &port_list, node)
	{
		state = port_get_state(link);
		if(state != PORT_STATE_CONN)
			continue;
		
		if(port->id != link->id && link->cb)
			link->cb(link, sk_buffer);
	}
	
	pthread_mutex_unlock(&port_list_mutex);

	return 1;
}

sk_buffer_t *port_recv(port_t *port)
{
	if(!port)
		return NULL;
	
	struct list_head *node = queue_pop(&port->queue);
	if(!node)
		return NULL;
	
	sk_buffer_t *sk_buffer = container_of(node, sk_buffer_t, node);
	if(!sk_buffer)
		return NULL;
	
	return sk_buffer;
}
