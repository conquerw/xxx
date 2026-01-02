/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include "kernel.h"
#include "murmur_hash3.h"
#include "util.h"
#include "sk_buffer.h"
#include "gateway.h"

static int gateway_broadcast_callback(port_t *port, sk_buffer_t *sk_buffer)
{
	if(!port || !sk_buffer)
		return -1;
	
	gateway_t *gateway = (gateway_t *)port_get_p(port);
	if(!gateway)
		return -1;
	
	if(gateway->middleware_ops)
		if(gateway->middleware_ops->send)
			gateway->middleware_ops->send(gateway->middleware_ops, PORT_BROADCAST, sk_buffer);

	return 1;
}

static int recv_callback(middleware_ops_t *middleware_ops, char *buffer, int buffer_len)
{
	if(!middleware_ops || !buffer || buffer_len == 0)
		return -1;
	
	gateway_t *gateway = (gateway_t *)middleware_ops_get_p(middleware_ops);
	if(!gateway)
		return -1;

	sk_buffer_t *sk_buffer = sk_buffer_create(0, buffer_len, 0);
	if(!sk_buffer)
		return -1;

	sk_buffer_data_copy(sk_buffer, buffer, buffer_len);
	queue_push(&gateway->queue, &sk_buffer->node);

	return 1;
}

int gateway_init(gateway_t *gateway, unsigned short id, char type)
{
	if(!gateway || id == PORT_UNKOWN || id == PORT_BROADCAST || type == MIDDLEWARE_TYPE_UNKOWN)
		return -1;
	
	gateway->id = id;
	gateway->type = type;
	
	queue_init(&gateway->queue, -1);
	gateway->port = port_create(gateway->id);
	if(!gateway->port)
		goto exit;
	
	port_set_broadcast_callback(gateway->port, gateway_broadcast_callback);
	port_set_p(gateway->port, gateway);

	sem_init(&gateway->port_sem, 0, 0);

	gateway->middleware_ops = middleware_create(gateway->type);
	if(!gateway->middleware_ops)
		goto exit2;
	
	middleware_ops_set_p(gateway->middleware_ops, gateway);
	
	if(gateway->middleware_ops->init)
		gateway->middleware_ops->init(gateway->middleware_ops, gateway->id);
	if(gateway->middleware_ops->set_recv_callback)
		gateway->middleware_ops->set_recv_callback(gateway->middleware_ops, recv_callback);

	sem_init(&gateway->middleware_ops_sem, 0, 0);

	return 1;
	
exit2:
	sem_destroy(&gateway->port_sem);
	port_destroy(gateway->port);
exit:
	queue_exit(&gateway->queue);
	
	return -1;
}

int gateway_exit(gateway_t *gateway)
{
	if(!gateway || !gateway->port || !gateway->middleware_ops)
		return -1;

	sem_destroy(&gateway->middleware_ops_sem);

	if(gateway->middleware_ops->exit)
		gateway->middleware_ops->exit(gateway->middleware_ops);
	middleware_destroy(gateway->middleware_ops, gateway->type);

	sem_destroy(&gateway->port_sem);

	port_destroy(gateway->port);
	queue_exit(&gateway->queue);
	
	return 1;
}

gateway_t *gateway_create(unsigned short id, char type)
{
	if(id == PORT_UNKOWN || id == PORT_BROADCAST || type == MIDDLEWARE_TYPE_UNKOWN)
		return NULL;
	
	gateway_t *gateway = (gateway_t *)calloc(1, sizeof(gateway_t));
	if(!gateway)
		return NULL;
	
	int ret = gateway_init(gateway, id, type);
	if(ret != 1)
	{
		free(gateway);
		return NULL;
	}
	
	return gateway;
}

int gateway_destroy(gateway_t *gateway)
{
	if(!gateway)
		return -1;
	
	gateway_exit(gateway);
	free(gateway);
	
	return 1;
}

static void *port_thread(void *para)
{
	if(!para)
		return NULL;
	
	gateway_t *gateway = (gateway_t *)para;
	if(!gateway->port || !gateway->middleware_ops)
		return NULL;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	
	port_set_state(gateway->port, PORT_STATE_CONN);

	sem_post(&gateway->port_sem);

	while(1)
	{
		sk_buffer_t *sk_buffer = port_recv(gateway->port);
		if(!sk_buffer)
			continue;

		unsigned int hash = *(unsigned int *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 4);
		unsigned int hash2 = 0;
		murmur_hash3_x86_32(sk_buffer->data, sk_buffer->tail - sk_buffer->data, 1, &hash2);
		if(hash != hash2)
		{
			printf("hash values are not equal, hash-hash2: %x-%x\n", hash, hash2);
			sk_buffer_destroy(sk_buffer);
			continue;
		}

		unsigned short source = *(unsigned short *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 2);
		unsigned short dest = *(unsigned short *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 2);
		sk_buffer_push_copy(sk_buffer, (char *)&dest, 2);
		sk_buffer_push_copy(sk_buffer, (char *)&source, 2);
		sk_buffer_push_copy(sk_buffer, (char *)&hash, 4);

		unsigned short next = port_get_next_jump(gateway->id, dest);
		if(next != PORT_UNKOWN)
		{
			if(gateway->middleware_ops->send)
				gateway->middleware_ops->send(gateway->middleware_ops, next, sk_buffer);
		}

		sk_buffer_destroy(sk_buffer);
	}
	
	return NULL;
}

static void *middleware_ops_thread(void *para)
{
	if(!para)
		return NULL;

	gateway_t *gateway = (gateway_t *)para;
	if(!gateway->port || !gateway->middleware_ops)
		return NULL;
	
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	sem_post(&gateway->middleware_ops_sem);

	while(1)
	{
		struct list_head *node = queue_pop(&gateway->queue);
		if(!node)
			continue;
		
		sk_buffer_t *sk_buffer = container_of(node, sk_buffer_t, node);
		if(!sk_buffer)
			continue;

		unsigned int hash = *(unsigned int *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 4);
		unsigned int hash2 = 0;
		murmur_hash3_x86_32(sk_buffer->data, sk_buffer->tail - sk_buffer->data, 1, &hash2);
		if(hash != hash2)
		{
			printf("hash values are not equal, hash-hash2: %x-%x\n", hash, hash2);
			sk_buffer_destroy(sk_buffer);
			continue;
		}

		unsigned short source = *(unsigned short *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 2);
		unsigned short dest = *(unsigned short *)sk_buffer->data;
		sk_buffer_pull(sk_buffer, 2);
		sk_buffer_push_copy(sk_buffer, (char *)&dest, 2);
		sk_buffer_push_copy(sk_buffer, (char *)&source, 2);
		sk_buffer_push_copy(sk_buffer, (char *)&hash, 4);
		
		if(dest == PORT_BROADCAST)
		{
			port_broadcast(gateway->port, sk_buffer);
			sk_buffer_destroy(sk_buffer);
		}
		else
		{
			int ret = port_send(gateway->port, dest, sk_buffer);
			if(ret == -1)
				sk_buffer_destroy(sk_buffer);
		}
	}
	
	return NULL;
}

int gateway_start(gateway_t *gateway)
{
	if(!gateway)
		return -1;

	set_root_caps();

    pthread_attr_t attr;
    struct sched_param param;

	pthread_attr_init(&attr);
	pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&attr, SCHED_RR);
	param.sched_priority = 80;
	pthread_attr_setschedparam(&attr, &param);

	pthread_create(&gateway->port_pthread, &attr, port_thread, gateway);
	pthread_detach(gateway->port_pthread);
	sem_wait(&gateway->port_sem);

	pthread_create(&gateway->middleware_ops_pthread, &attr, middleware_ops_thread, gateway);
	pthread_detach(gateway->middleware_ops_pthread);
	sem_wait(&gateway->middleware_ops_sem);

	pthread_attr_destroy(&attr);
	
	if(gateway->middleware_ops->start)
		gateway->middleware_ops->start(gateway->middleware_ops);

	return 1;
}

int gateway_stop(gateway_t *gateway)
{
	if(!gateway)
		return -1;
	
	if(gateway->middleware_ops->stop)
		gateway->middleware_ops->stop(gateway->middleware_ops);

	pthread_cancel(gateway->middleware_ops_pthread);
	port_set_state(gateway->port, PORT_STATE_DISCONN);
	pthread_cancel(gateway->port_pthread);
	
	return 1;
}
