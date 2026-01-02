/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include "kernel.h"
#include "port.h"
#include "middleware_iox2.h"

#define FRAME_LEN 307200

typedef struct
{
    unsigned int type : 1;
    unsigned int len : 31;
    char frame[FRAME_LEN];
} packet_t;

typedef struct
{
	char id[16];
	iox2_service_name_h name;
	iox2_port_factory_pub_sub_h service;
	iox2_publisher_h publisher;
	iox2_port_factory_event_h event_service;
	iox2_notifier_h notifier;
	middleware_iox2_t *middleware_iox2;
	struct list_head node;
} iox2_pub_t;

int middleware_iox2_init(middleware_ops_t *middleware_ops, unsigned short id)
{
	if(!middleware_ops)
		return -1;

	middleware_iox2_t *middleware_iox2 = container_of(middleware_ops, middleware_iox2_t, middleware_ops);
	if(!middleware_iox2)
		return -1;

	iox2_set_log_level_from_env_or(iox2_log_level_e_ERROR);

	iox2_node_builder_h node_builder_handle = iox2_node_builder_new(NULL);
	int ret = iox2_node_builder_create(node_builder_handle, NULL, iox2_service_type_e_IPC, &middleware_iox2->node_handle);
	if(ret != IOX2_OK)
	{
		printf("iox2_node_builder_create is failed: %d\n", ret);
		return -1;
	}

	snprintf(middleware_iox2->iox2_sub[0].id, sizeof(middleware_iox2->iox2_sub[0].id), "process%05d", id);
	memcpy(middleware_iox2->iox2_sub[1].id, "process65535", strlen("process65535"));

	pthread_mutex_init(&middleware_iox2->cb_mutex, NULL);
	INIT_LIST_HEAD(&middleware_iox2->node);
	pthread_mutex_init(&middleware_iox2->node_mutex, NULL);

	return 1;
}

static int iox2_pub_exit(iox2_pub_t *iox2_pub)
{
	if(!iox2_pub)
		return -1;

	iox2_notifier_drop(iox2_pub->notifier);
    iox2_publisher_drop(iox2_pub->publisher);
	iox2_port_factory_event_drop(iox2_pub->event_service);
	iox2_port_factory_pub_sub_drop(iox2_pub->service);
	iox2_service_name_drop(iox2_pub->name);

	return 1;
}

static int iox2_sub_exit(iox2_sub_t *iox2_sub)
{
	if(!iox2_sub)
		return -1;

	iox2_listener_drop(iox2_sub->listener);
    iox2_subscriber_drop(iox2_sub->subscriber);
	iox2_port_factory_event_drop(iox2_sub->event_service);
	iox2_port_factory_pub_sub_drop(iox2_sub->service);
	iox2_service_name_drop(iox2_sub->name);

	return 1;
}

int middleware_iox2_exit(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;
	
	middleware_iox2_t *middleware_iox2 = container_of(middleware_ops, middleware_iox2_t, middleware_ops);
	if(!middleware_iox2)
		return -1;

	pthread_mutex_destroy(&middleware_iox2->node_mutex);

	iox2_pub_t *link = NULL, *next = NULL;
	list_for_each_entry_safe(link, next, &middleware_iox2->node, node)
	{
		list_del(&link->node);
		iox2_pub_exit(link);
		free(link);
	}

	pthread_mutex_destroy(&middleware_iox2->cb_mutex);

	for(int i = 0; i < 2; i++)
		iox2_sub_exit(&middleware_iox2->iox2_sub[i]);

	iox2_node_drop(middleware_iox2->node_handle);

	return 1;
}

static int iox2_pub_init(iox2_pub_t *iox2_pub)
{
	if(!iox2_pub)
		return -1;

	int ret = iox2_service_name_new(NULL, iox2_pub->id, strlen(iox2_pub->id), &iox2_pub->name);
	if(ret != IOX2_OK)
	{
		printf("iox2_service_name_new is failed: %d\n", ret);
		return -1;
	}

	iox2_service_name_ptr service_name_ptr = iox2_cast_service_name_ptr(iox2_pub->name);
	iox2_service_builder_h service_builder = iox2_node_service_builder(&iox2_pub->middleware_iox2->node_handle, NULL, service_name_ptr);
	iox2_service_builder_pub_sub_h service_builder_pub_sub = iox2_service_builder_pub_sub(service_builder);

	ret = iox2_service_builder_pub_sub_set_payload_type_details(&service_builder_pub_sub, iox2_type_variant_e_FIXED_SIZE, iox2_pub->id, strlen(iox2_pub->id), sizeof(packet_t), alignof(packet_t));
	if(ret != IOX2_OK)
	{
		printf("iox2_service_builder_pub_sub_set_payload_type_details is failed: %d\n", ret);
		goto name_exit;
	}

	iox2_service_builder_pub_sub_set_history_size(&service_builder_pub_sub, 0);
	iox2_service_builder_pub_sub_set_subscriber_max_buffer_size(&service_builder_pub_sub, 100);
	iox2_service_builder_pub_sub_set_enable_safe_overflow(&service_builder_pub_sub, 1);
	iox2_service_builder_pub_sub_set_subscriber_max_borrowed_samples(&service_builder_pub_sub, 3);

	ret = iox2_service_builder_pub_sub_open_or_create(service_builder_pub_sub, NULL, &iox2_pub->service);
	if(ret != IOX2_OK)
	{
		printf("iox2_service_builder_pub_sub_open_or_create is failed: %d\n", ret);
		goto name_exit;
	}

	iox2_service_name_ptr service_name_ptr2 = iox2_cast_service_name_ptr(iox2_pub->name);
	iox2_service_builder_h service_builder2 = iox2_node_service_builder(&iox2_pub->middleware_iox2->node_handle, NULL, service_name_ptr2);
	iox2_service_builder_event_h service_builder_event = iox2_service_builder_event(service_builder2);

	ret = iox2_service_builder_event_open_or_create(service_builder_event, NULL, &iox2_pub->event_service);
	if(ret != IOX2_OK)
	{
		printf("iox2_service_builder_event_open_or_create is failed: %d\n", ret);
		goto service_exit;
	}

	iox2_port_factory_publisher_builder_h publisher_builder = iox2_port_factory_pub_sub_publisher_builder(&iox2_pub->service, NULL);
	ret = iox2_port_factory_publisher_builder_create(publisher_builder, NULL, &iox2_pub->publisher);
	if(ret != IOX2_OK)
	{
		printf("iox2_port_factory_publisher_builder_create is failed: %d\n", ret);
		goto event_service_exit;
	}

	iox2_port_factory_notifier_builder_h notifier_builder = iox2_port_factory_event_notifier_builder(&iox2_pub->event_service, NULL);
	ret = iox2_port_factory_notifier_builder_create(notifier_builder, NULL, &iox2_pub->notifier);
	if(ret != IOX2_OK)
	{
		printf("iox2_port_factory_notifier_builder_create is failed: %d\n", ret);
		goto publisher_exit;
	}

	return 1;

publisher_exit:
    iox2_publisher_drop(iox2_pub->publisher);
event_service_exit:
	iox2_port_factory_event_drop(iox2_pub->event_service);
service_exit:
	iox2_port_factory_pub_sub_drop(iox2_pub->service);
name_exit:
	iox2_service_name_drop(iox2_pub->name);

	return -1;
}

static int iox2_send(iox2_pub_t *iox2_pub, char *buffer, int buffer_len)
{
	if(!iox2_pub || !buffer || buffer_len == 0)
		return -1;

    if(buffer_len <= FRAME_LEN)
    {
        iox2_sample_mut_h sample = NULL;
		int ret = iox2_publisher_loan_slice_uninit(&iox2_pub->publisher, NULL, &sample, 1);
        if(ret != IOX2_OK)
		{
			printf("iox2_publisher_loan_slice_uninit is failed: %d\n", ret);
			return -1;
        }
		
        packet_t *packet = NULL;
        iox2_sample_mut_payload_mut(&sample, (void**) &packet, NULL);

        packet->type = 0;
        packet->len = buffer_len;
        memcpy(packet->frame, buffer, buffer_len);

		ret = iox2_sample_mut_send(sample, NULL);
        if(ret != IOX2_OK)
		{
			printf("iox2_sample_mut_send is failed: %d\n", ret);
			return -1;
        }
    }
    else
    {
        int remain_len = buffer_len - (FRAME_LEN - 6);
        int remainder = remain_len % (FRAME_LEN - 1);
        unsigned char package_num = remain_len / (FRAME_LEN - 1);

        package_num += 1;
        if(remainder != 0)
            package_num += 1;

        // printf("package_num, remainder: %d-%d\n", package_num, remainder);

        int index = 0;
        for(unsigned char i = 0; i < package_num; i++)
        {
            iox2_sample_mut_h sample = NULL;
			int ret = iox2_publisher_loan_slice_uninit(&iox2_pub->publisher, NULL, &sample, 1);
			if(ret != IOX2_OK)
			{
				printf("iox2_publisher_loan_slice_uninit is failed: %d\n", ret);
				return -1;
			}

            packet_t *packet = NULL;
            iox2_sample_mut_payload_mut(&sample, (void**) &packet, NULL);

            packet->type = 1;
            packet->frame[0] = i;

            if(i == 0)
            {
                packet->len = FRAME_LEN;
                *((unsigned int *)(packet->frame + 1)) = buffer_len;
                packet->frame[5] = package_num;
                memcpy(packet->frame + 6, buffer, FRAME_LEN - 6);
                index += FRAME_LEN - 6;
            }
            else
            {
                if(remainder != 0 && i == (package_num - 1))
                {
                    packet->len = buffer_len - index + 1;
                    memcpy(packet->frame + 1, buffer + index, buffer_len - index);
                    index = buffer_len;
                }
                else
                {
                    packet->len = FRAME_LEN;
                    memcpy(packet->frame + 1, buffer + index, FRAME_LEN - 1);
                    index += FRAME_LEN - 1;
                }
            }
			
			ret = iox2_sample_mut_send(sample, NULL);
			if(ret != IOX2_OK)
			{
				printf("iox2_sample_mut_send is failed: %d\n", ret);
				return -1;
			}
        }
    }

	iox2_event_id_t iox2_event_id = { .value = 5 };
	int ret = iox2_notifier_notify_with_custom_event_id(&iox2_pub->notifier, &iox2_event_id, NULL);
	if(ret != IOX2_OK)
	{
		printf("iox2_notifier_notify_with_custom_event_id is failed: %d\n", ret);
		return -1;
	}

	return 1;
}

int middleware_iox2_send(middleware_ops_t *middleware_ops, unsigned short id, sk_buffer_t *sk_buffer)
{
	if(!middleware_ops || id == PORT_UNKOWN || !sk_buffer)
		return -1;
	
	middleware_iox2_t *middleware_iox2 = container_of(middleware_ops, middleware_iox2_t, middleware_ops);
	if(!middleware_iox2)
		return -1;

	iox2_pub_t *iox2_pub = NULL;

	char topic[16] = "";
	memset(topic, 0x00, sizeof(topic));
	snprintf(topic, sizeof(topic), "process%05d", id);

	pthread_mutex_lock(&middleware_iox2->node_mutex);

	iox2_pub_t *link = NULL;
	list_for_each_entry(link, &middleware_iox2->node, node)
	{
		if(memcmp(link->id, topic, strlen(topic)) == 0)
		{
			iox2_pub = link;
			break;
		}
	}

	if(!iox2_pub)
	{
		iox2_pub = (iox2_pub_t *)calloc(1, sizeof(iox2_pub_t));
		if(!iox2_pub)
			goto exit;

		memcpy(iox2_pub->id, topic, strlen(topic));
		iox2_pub->middleware_iox2 = middleware_iox2;
		iox2_pub_init(iox2_pub);
		list_add_tail(&iox2_pub->node, &middleware_iox2->node);
	}

	iox2_send(iox2_pub, sk_buffer->data, sk_buffer->tail - sk_buffer->data);

exit:
	pthread_mutex_unlock(&middleware_iox2->node_mutex);

	return 1;
}

static int iox2_sub_init(iox2_sub_t *iox2_sub)
{
	if(!iox2_sub)
		return -1;

	int ret = iox2_service_name_new(NULL, iox2_sub->id, strlen(iox2_sub->id), &iox2_sub->name);
	if(ret != IOX2_OK)
	{
		printf("iox2_service_name_new is failed: %d\n", ret);
		return -1;
	}

	iox2_service_name_ptr service_name_ptr = iox2_cast_service_name_ptr(iox2_sub->name);
	iox2_service_builder_h service_builder = iox2_node_service_builder(&iox2_sub->middleware_iox2->node_handle, NULL, service_name_ptr);
	iox2_service_builder_pub_sub_h service_builder_pub_sub = iox2_service_builder_pub_sub(service_builder);

	ret = iox2_service_builder_pub_sub_set_payload_type_details(&service_builder_pub_sub, iox2_type_variant_e_FIXED_SIZE, iox2_sub->id, strlen(iox2_sub->id), sizeof(packet_t), alignof(packet_t));
	if(ret != IOX2_OK)
	{
		printf("iox2_service_builder_pub_sub_set_payload_type_details is failed: %d\n", ret);
		goto name_exit;
	}

	iox2_service_builder_pub_sub_set_history_size(&service_builder_pub_sub, 0);
	iox2_service_builder_pub_sub_set_subscriber_max_buffer_size(&service_builder_pub_sub, 100);
	iox2_service_builder_pub_sub_set_enable_safe_overflow(&service_builder_pub_sub, 1);
	iox2_service_builder_pub_sub_set_subscriber_max_borrowed_samples(&service_builder_pub_sub, 3);

	ret = iox2_service_builder_pub_sub_open_or_create(service_builder_pub_sub, NULL, &iox2_sub->service);
	if(ret != IOX2_OK)
	{
		printf("iox2_service_builder_pub_sub_open_or_create is failed: %d\n", ret);
		goto name_exit;
	}

	iox2_service_name_ptr service_name_ptr2 = iox2_cast_service_name_ptr(iox2_sub->name);
	iox2_service_builder_h service_builder2 = iox2_node_service_builder(&iox2_sub->middleware_iox2->node_handle, NULL, service_name_ptr2);
	iox2_service_builder_event_h service_builder_event = iox2_service_builder_event(service_builder2);

	ret = iox2_service_builder_event_open_or_create(service_builder_event, NULL, &iox2_sub->event_service);
	if(ret != IOX2_OK)
	{
		printf("iox2_service_builder_event_open_or_create is failed: %d\n", ret);
		goto service_exit;
	}

	iox2_port_factory_subscriber_builder_h subscriber_builder = iox2_port_factory_pub_sub_subscriber_builder(&iox2_sub->service, NULL);
	ret = iox2_port_factory_subscriber_builder_create(subscriber_builder, NULL, &iox2_sub->subscriber);
	if(ret != IOX2_OK)
	{
		printf("iox2_port_factory_subscriber_builder_create is failed: %d\n", ret);
		goto event_service_exit;
	}

	iox2_port_factory_listener_builder_h listener_builder = iox2_port_factory_event_listener_builder(&iox2_sub->event_service, NULL);
	ret = iox2_port_factory_listener_builder_create(listener_builder, NULL, &iox2_sub->listener);
	if(ret != IOX2_OK)
	{
		printf("iox2_port_factory_listener_builder_create is failed: %d\n", ret);
		goto subscriber_exit;
	}

	return 1;

subscriber_exit:
    iox2_subscriber_drop(iox2_sub->subscriber);
event_service_exit:
	iox2_port_factory_event_drop(iox2_sub->event_service);
service_exit:
	iox2_port_factory_pub_sub_drop(iox2_sub->service);
name_exit:
	iox2_service_name_drop(iox2_sub->name);

	return -1;
}

int middleware_iox2_set_recv_callback(middleware_ops_t *middleware_ops, recv_callback_t cb)
{
	if(!middleware_ops || !cb)
		return -1;
	
	middleware_iox2_t *middleware_iox2 = container_of(middleware_ops, middleware_iox2_t, middleware_ops);
	if(!middleware_iox2)
		return -1;

	pthread_mutex_lock(&middleware_iox2->cb_mutex);
	middleware_iox2->cb = cb;
	pthread_mutex_unlock(&middleware_iox2->cb_mutex);

	for(int i = 0; i < 2; i++)
	{
		sem_init(&middleware_iox2->iox2_sub[i].sem, 0, 0);
		middleware_iox2->iox2_sub[i].middleware_iox2 = middleware_iox2;
		iox2_sub_init(&middleware_iox2->iox2_sub[i]);
	}

	return 1;
}

static void *iox2_thread(void *para)
{
	if(!para)
		return NULL;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	iox2_sub_t *iox2_sub = (iox2_sub_t *)para;

	sem_post(&iox2_sub->sem);

	while(1)
	{
		iox2_event_id_t iox2_event_id;
        bool is_wait_one = false;
        iox2_listener_blocking_wait_one(&iox2_sub->listener, &iox2_event_id, &is_wait_one);
        if(is_wait_one)
        {
            iox2_sample_h sample = NULL;
            while(iox2_subscriber_receive(&iox2_sub->subscriber, NULL, &sample) == IOX2_OK)
            {
                if(sample)
				{
                    packet_t *packet = NULL;
                    iox2_sample_payload(&sample, (const void**) &packet, NULL);
                    if(packet->type == 0)
					{
						pthread_mutex_lock(&iox2_sub->middleware_iox2->cb_mutex);
						if(iox2_sub->middleware_iox2->cb)
							iox2_sub->middleware_iox2->cb(&iox2_sub->middleware_iox2->middleware_ops, packet->frame, packet->len);
						pthread_mutex_unlock(&iox2_sub->middleware_iox2->cb_mutex);
					}
                    // else if(packet->type == 1)
                    // {
                    //     printf("received: %d-%d-%d\n", packet->type, packet->len, packet->frame[0]);
                    // }

                    iox2_sample_drop(sample);
                }
                else
                    break;
            }
		}
	}

	return NULL;
}

int middleware_iox2_start(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;

	middleware_iox2_t *middleware_iox2 = container_of(middleware_ops, middleware_iox2_t, middleware_ops);
	if(!middleware_iox2)
		return -1;

	for(int i = 0; i < 2; i++)
	{
		pthread_create(&middleware_iox2->iox2_sub[i].pthread, NULL, iox2_thread, &middleware_iox2->iox2_sub[i]);
		pthread_detach(middleware_iox2->iox2_sub[i].pthread);
		sem_wait(&middleware_iox2->iox2_sub[i].sem);
	}

	return 1;
}

int middleware_iox2_stop(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;
	
	middleware_iox2_t *middleware_iox2 = container_of(middleware_ops, middleware_iox2_t, middleware_ops);
	if(!middleware_iox2)
		return -1;

	for(int i = 0; i < 2; i++)
		pthread_cancel(middleware_iox2->iox2_sub[i].pthread);

	return 1;
}
