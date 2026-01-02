/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "nng/nng.h"
#include "nng/protocol/pubsub0/pub.h"
#include "nng/protocol/pubsub0/sub.h"
#include "kernel.h"
#include "port.h"
#include "middleware_nng.h"

#define PORT_ID_LEN 5

typedef struct
{
	unsigned short id;
	nng_socket sock;
	recv_callback_t cb;
	pthread_t pthread;
	middleware_nng_t *middleware_nng;
	struct list_head node;
} nng_node_t;

static unsigned short port_id[PORT_ID_LEN] = {PORT_MCU_ROUTE, PORT_MPU_ROUTE, PORT_MPU_TEST_ROUTE, PORT_MPU_TEST2_ROUTE, PORT_MPU_TEST3_ROUTE};

int middleware_nng_init(middleware_ops_t *middleware_ops, unsigned short id)
{
	if(!middleware_ops)
		return -1;
	
	middleware_nng_t *middleware_nng = container_of(middleware_ops, middleware_nng_t, middleware_ops);
	if(!middleware_nng)
		return -1;

	middleware_nng->nng.id = id;
	
    int ret = nng_pub0_open(&middleware_nng->nng.sock);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
        return -1;
    }

	char url[32] = "";
	memset(url, 0x00, sizeof(url));
	snprintf(url, sizeof(url), "ipc:///tmp/%05d", id);
	ret = nng_listen(middleware_nng->nng.sock, url, NULL, 0);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
		nng_close(middleware_nng->nng.sock);
		return -1;
    }

	INIT_LIST_HEAD(&middleware_nng->node);
	pthread_mutex_init(&middleware_nng->node_mutex, NULL);

	return 1;
}

int middleware_nng_exit(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;
	
	middleware_nng_t *middleware_nng = container_of(middleware_ops, middleware_nng_t, middleware_ops);
	if(!middleware_nng)
		return -1;

	pthread_mutex_destroy(&middleware_nng->node_mutex);

	nng_node_t *link = NULL, *next = NULL;
	list_for_each_entry_safe(link, next, &middleware_nng->node, node)
	{
		list_del(&link->node);
		free(link);
	}

	nng_close(middleware_nng->nng.sock);

	return 1;
}

int middleware_nng_send(middleware_ops_t *middleware_ops, unsigned short id, sk_buffer_t *sk_buffer)
{
	if(!middleware_ops || id == PORT_UNKOWN || !sk_buffer)
		return -1;
	
	middleware_nng_t *middleware_nng = container_of(middleware_ops, middleware_nng_t, middleware_ops);
	if(!middleware_nng)
		return -1;

	if(id != PORT_BROADCAST)
	{
		char topic[16] = "";
		memset(topic, 0x00, sizeof(topic));
		snprintf(topic, sizeof(topic), "process%05d", id);

		sk_buffer_push_copy(sk_buffer, topic, strlen(topic));
		nng_send(middleware_nng->nng.sock, sk_buffer->data, sk_buffer->tail - sk_buffer->data, 0);
	}
	else if(id == PORT_BROADCAST)
	{
		for(int i = 0; i < PORT_ID_LEN; i++)
		{
			if(middleware_nng->nng.id != port_id[i])
			{
				char topic[16] = "";
				memset(topic, 0x00, sizeof(topic));
				snprintf(topic, sizeof(topic), "process%05d", id);

				sk_buffer_push_copy(sk_buffer, topic, strlen(topic));
				nng_send(middleware_nng->nng.sock, sk_buffer->data, sk_buffer->tail - sk_buffer->data, 0);
				sk_buffer_pull(sk_buffer, strlen(topic));
			}
		}
	}

	return 1;
}

int middleware_nng_set_recv_callback(middleware_ops_t *middleware_ops, recv_callback_t cb)
{
	if(!middleware_ops || !cb)
		return -1;
	
	middleware_nng_t *middleware_nng = container_of(middleware_ops, middleware_nng_t, middleware_ops);
	if(!middleware_nng)
		return -1;

	for(int i = 0; i < PORT_ID_LEN; i++)
	{
		if(middleware_nng->nng.id != port_id[i])
		{
			nng_node_t *nng_node = (nng_node_t *)calloc(1, sizeof(nng_node_t));
			if(!nng_node)
				goto exit;

			nng_node->id = port_id[i];
			nng_node->cb = cb;
			nng_node->middleware_nng = middleware_nng;

			list_add_tail(&nng_node->node, &middleware_nng->node);
		}
	}

	return 1;

exit:
	{
		nng_node_t *link = NULL, *next = NULL;
		list_for_each_entry_safe(link, next, &middleware_nng->node, node)
		{
			list_del(&link->node);
			free(link);
		}
	}

	return -1;
}

static void *nng_thread(void *para)
{
	if(!para)
		return NULL;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	nng_node_t *nng_node = (nng_node_t *)para;

	int ret = nng_sub0_open(&nng_node->sock);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
		return NULL;
	}

	char topic[16] = "";
	memset(topic, 0x00, sizeof(topic));
	snprintf(topic, sizeof(topic), "process%05d", nng_node->middleware_nng->nng.id);
	int len = strlen(topic);
	ret = nng_setopt(nng_node->sock, NNG_OPT_SUB_SUBSCRIBE, topic, len);
	if(ret != 0)
	{
		printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
		return NULL;
	}

	char url[32] = "";
	memset(url, 0x00, sizeof(url));
	snprintf(url, sizeof(url), "ipc:///tmp/%05d", nng_node->id);
	
	while(1)
	{
		ret = -1;

		while(ret != 0)
		{
			ret = nng_dial(nng_node->sock, url, NULL, 0);
			if(ret != 0)
			{
				// printf("%s-%d: %s\n", __func__, __LINE__, nng_strerror(ret));
				usleep(500 * 1000);
			}
		}
		
		while(1)
		{
			char *buffer = NULL;
			size_t buffer_len = 0;
			nng_recv(nng_node->sock, &buffer, &buffer_len, NNG_FLAG_ALLOC);
			
			ret = memcmp(buffer, topic, len);
			if(ret == 0)
			{
				pthread_mutex_lock(&nng_node->middleware_nng->node_mutex);

				if(nng_node->cb)
					nng_node->cb(&nng_node->middleware_nng->middleware_ops, buffer + len, buffer_len - len);

				pthread_mutex_unlock(&nng_node->middleware_nng->node_mutex);
			}

			nng_free(buffer, buffer_len);
		}
	}

	return NULL;
}

int middleware_nng_start(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;

	middleware_nng_t *middleware_nng = container_of(middleware_ops, middleware_nng_t, middleware_ops);
	if(!middleware_nng)
		return -1;

	nng_node_t *link = NULL;
	list_for_each_entry(link, &middleware_nng->node, node)
	{
		pthread_create(&link->pthread, NULL, nng_thread, link);
		pthread_detach(link->pthread);
	}

	return 1;
}

int middleware_nng_stop(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;

	middleware_nng_t *middleware_nng = container_of(middleware_ops, middleware_nng_t, middleware_ops);
	if(!middleware_nng)
		return -1;

	nng_node_t *link = NULL;
	list_for_each_entry(link, &middleware_nng->node, node)
	{
		nng_close(link->sock);
		pthread_cancel(link->pthread);
	}

	return 1;
}
