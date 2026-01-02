/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "iox2/iceoryx2.h"
#include "list.h"

#define FRAME_LEN 307200

typedef struct
{
    unsigned int type : 1;
    unsigned int len : 31;
    char frame[FRAME_LEN];
} packet_t;

static iox2_publisher_h publisher = NULL;
static iox2_notifier_h notifier = NULL;
static iox2_subscriber_h subscriber = NULL;
static iox2_listener_h listener = NULL;

// test protocol
// length + send_count + recv_count + bounce_time

#define BYTE_LEN 6
#define SAMPLE_LEN 1000

typedef struct
{
	double time;
	struct list_head list;
} time_node_t;

typedef struct
{
	unsigned short send_count;
	unsigned short recv_count;
	double total_time;
	double standard_deviation_time;
	double average_time;
	double min_time;
	double percentile_30_time;
	double percentile_50_time;
	double percentile_90_time;
	double percentile_99_time;
	double percentile_9999_time;
	double max_time;
	struct list_head node;
} time_result_t;

static unsigned int byte[BYTE_LEN] = {64, 256, 1024, 4096, 16384, 65536};
static time_result_t time_result[BYTE_LEN];
static int i = 0;
static double start = 0;
static pthread_mutex_t mutex;
static pthread_condattr_t condattr;
static pthread_cond_t cond;
static int start_flag = 0;

static double get_microsecond(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);

	return (double)time.tv_sec * 1000 + (double)time.tv_nsec / 1000000;
}

static void swap_nodes(time_node_t *a, time_node_t *b)
{
	if(!a || !b)
		return;

    struct list_head *a_prev = a->list.prev;
    struct list_head *a_next = a->list.next;
    struct list_head *b_prev = b->list.prev;
    struct list_head *b_next = b->list.next;

    list_del(&a->list);
    list_del(&b->list);

    if(a_next == &b->list)
	{
        __list_add(&b->list, a_prev, &a->list);
        __list_add(&a->list, &b->list, b_next);
    }
	else if(b_next == &a->list)
	{
        __list_add(&a->list, b_prev, &b->list);
        __list_add(&b->list, &a->list, a_next);
    }
	else
	{
        __list_add(&a->list, b_prev, b_next);
        __list_add(&b->list, a_prev, a_next);
    }
}

static void bubble_sort_list(struct list_head *head)
{
	if(!head)
		return;

    char swapped;
    struct list_head *p;
    do
	{
        swapped = 0;
        p = head->next;
        while(p->next != head)
		{
            time_node_t *node1 = list_entry(p, time_node_t, list);
            time_node_t *node2 = list_entry(p->next, time_node_t, list);
            if(node1->time > node2->time)
			{
                swap_nodes(node1, node2);
                swapped = 1;
                break;
            }
			else
				p = p->next;
        }
    } while(swapped);
}

static int iox2_send(char *buffer, int buffer_len)
{
	if(!buffer || buffer_len == 0)
		return -1;

    if(buffer_len <= FRAME_LEN)
    {
        iox2_sample_mut_h sample = NULL;
		int ret = iox2_publisher_loan_slice_uninit(&publisher, NULL, &sample, 1);
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
			int ret = iox2_publisher_loan_slice_uninit(&publisher, NULL, &sample, 1);
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
	int ret = iox2_notifier_notify_with_custom_event_id(&notifier, &iox2_event_id, NULL);
	if(ret != IOX2_OK)
	{
		printf("iox2_notifier_notify_with_custom_event_id is failed: %d\n", ret);
		return -1;
	}

	return 1;
}

static void *iox2_thread(void *para)
{
	while(1)
	{
		iox2_event_id_t iox2_event_id;
        bool is_wait_one = false;
        iox2_listener_blocking_wait_one(&listener, &iox2_event_id, &is_wait_one);
        if(is_wait_one)
        {
            iox2_sample_h sample = NULL;
            while(iox2_subscriber_receive(&subscriber, NULL, &sample) == IOX2_OK)
            {
                if(sample)
				{
                    packet_t *packet = NULL;
                    iox2_sample_payload(&sample, (const void**) &packet, NULL);
                    if(packet->type == 0)
					{
						char *buffer = packet->frame;

						double end = get_microsecond();

						time_result[i].recv_count = *((unsigned int *)(buffer + 8));
						float bounce_time = *((float *)(buffer + 12));

						time_node_t *time_node = (time_node_t *)calloc(1, sizeof(time_node_t));
						if(!time_node)
							goto exit;

						time_node->time = (end - start - bounce_time) / 2;
						list_add_tail(&time_node->list, &time_result[i].node);

					exit:
						pthread_mutex_lock(&mutex);
						start_flag = 1;
						pthread_mutex_unlock(&mutex);
						pthread_cond_signal(&cond);
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

int main(int argc, char *argv[])
{
    iox2_set_log_level_from_env_or(iox2_log_level_e_ERROR);

	iox2_config_ptr config_ptr = iox2_config_global_config();
	iox2_config_h config = NULL;
	iox2_config_from_ptr(config_ptr, NULL, &config);

    printf("iox2_config_defaults_publish_subscribe_publisher_history_size: %ld\n", iox2_config_defaults_publish_subscribe_publisher_history_size(&config));
    printf("iox2_config_defaults_publish_subscribe_subscriber_max_buffer_size: %ld\n", iox2_config_defaults_publish_subscribe_subscriber_max_buffer_size(&config));
	printf("iox2_config_defaults_publish_subscribe_enable_safe_overflow: %d\n", iox2_config_defaults_publish_subscribe_enable_safe_overflow(&config));
    printf("iox2_config_defaults_publish_subscribe_subscriber_max_borrowed_samples: %ld\n", iox2_config_defaults_publish_subscribe_subscriber_max_borrowed_samples(&config));

	iox2_node_builder_h node_builder_handle = iox2_node_builder_new(NULL);
	iox2_node_h node_handle = NULL;
	int ret = iox2_node_builder_create(node_builder_handle, NULL, iox2_service_type_e_IPC, &node_handle);
	if(ret != IOX2_OK)
	{
		printf("iox2_node_builder_create is failed: %d\n", ret);
		return -1;
	}

	{
		const char *service_name_value = "process123";
		iox2_service_name_h service_name = NULL;
		ret = iox2_service_name_new(NULL, service_name_value, strlen(service_name_value), &service_name);
		if(ret != IOX2_OK)
		{
			printf("iox2_service_name_new is failed: %d\n", ret);
			return -1;
		}

		iox2_service_name_ptr service_name_ptr = iox2_cast_service_name_ptr(service_name);
		iox2_service_builder_h service_builder = iox2_node_service_builder(&node_handle, NULL, service_name_ptr);
		iox2_service_builder_pub_sub_h service_builder_pub_sub = iox2_service_builder_pub_sub(service_builder);

		ret = iox2_service_builder_pub_sub_set_payload_type_details(&service_builder_pub_sub, iox2_type_variant_e_FIXED_SIZE, service_name_value, strlen(service_name_value), sizeof(packet_t), alignof(packet_t));
		if(ret != IOX2_OK)
		{
			printf("iox2_service_builder_pub_sub_set_payload_type_details is failed: %d\n", ret);
			return -1;
		}

		iox2_service_builder_pub_sub_set_history_size(&service_builder_pub_sub, 0);
		iox2_service_builder_pub_sub_set_subscriber_max_buffer_size(&service_builder_pub_sub, 1);
		iox2_service_builder_pub_sub_set_enable_safe_overflow(&service_builder_pub_sub, 1);
		iox2_service_builder_pub_sub_set_subscriber_max_borrowed_samples(&service_builder_pub_sub, 3);

		iox2_port_factory_pub_sub_h pubsub_service = NULL;
		ret = iox2_service_builder_pub_sub_open_or_create(service_builder_pub_sub, NULL, &pubsub_service);
		if(ret != IOX2_OK)
		{
			printf("iox2_service_builder_pub_sub_open_or_create is failed: %d\n", ret);
			return -1;
		}

		iox2_port_factory_publisher_builder_h publisher_builder = iox2_port_factory_pub_sub_publisher_builder(&pubsub_service, NULL);
		ret = iox2_port_factory_publisher_builder_create(publisher_builder, NULL, &publisher);
		if(ret != IOX2_OK)
		{
			printf("iox2_port_factory_publisher_builder_create is failed: %d\n", ret);
			return -1;
		}

		iox2_service_name_ptr service_name_ptr2 = iox2_cast_service_name_ptr(service_name);
		iox2_service_builder_h service_builder2 = iox2_node_service_builder(&node_handle, NULL, service_name_ptr2);
		iox2_service_builder_event_h service_builder_event = iox2_service_builder_event(service_builder2);

		iox2_port_factory_event_h event_service = NULL;
		ret = iox2_service_builder_event_open_or_create(service_builder_event, NULL, &event_service);
		if(ret != IOX2_OK)
		{
			printf("iox2_service_builder_event_open_or_create is failed: %d\n", ret);
			return -1;
		}

		iox2_port_factory_notifier_builder_h notifier_builder = iox2_port_factory_event_notifier_builder(&event_service, NULL);
		ret = iox2_port_factory_notifier_builder_create(notifier_builder, NULL, &notifier);
		if(ret != IOX2_OK)
		{
			printf("iox2_port_factory_notifier_builder_create is failed: %d\n", ret);
			return -1;
		}
	}

	{
		const char *service_name_value = "1111111111";
		iox2_service_name_h service_name = NULL;
		ret = iox2_service_name_new(NULL, service_name_value, strlen(service_name_value), &service_name);
		if(ret != IOX2_OK)
		{
			printf("iox2_service_name_new is failed: %d\n", ret);
			return -1;
		}

		iox2_service_name_ptr service_name_ptr = iox2_cast_service_name_ptr(service_name);
		iox2_service_builder_h service_builder = iox2_node_service_builder(&node_handle, NULL, service_name_ptr);
		iox2_service_builder_pub_sub_h service_builder_pub_sub = iox2_service_builder_pub_sub(service_builder);

		ret = iox2_service_builder_pub_sub_set_payload_type_details(&service_builder_pub_sub, iox2_type_variant_e_FIXED_SIZE, service_name_value, strlen(service_name_value), sizeof(packet_t), alignof(packet_t));
		if(ret != IOX2_OK)
		{
			printf("iox2_service_builder_pub_sub_set_payload_type_details is failed: %d\n", ret);
			return -1;
		}

		iox2_service_builder_pub_sub_set_history_size(&service_builder_pub_sub, 0);
		iox2_service_builder_pub_sub_set_subscriber_max_buffer_size(&service_builder_pub_sub, 1);
		iox2_service_builder_pub_sub_set_enable_safe_overflow(&service_builder_pub_sub, 1);
		iox2_service_builder_pub_sub_set_subscriber_max_borrowed_samples(&service_builder_pub_sub, 3);

		iox2_port_factory_pub_sub_h pubsub_service = NULL;
		ret = iox2_service_builder_pub_sub_open_or_create(service_builder_pub_sub, NULL, &pubsub_service );
		if(ret != IOX2_OK)
		{
			printf("iox2_service_builder_pub_sub_open_or_create is failed: %d\n", ret);
			return -1;
		}

		iox2_port_factory_subscriber_builder_h subscriber_builder = iox2_port_factory_pub_sub_subscriber_builder(&pubsub_service , NULL);
		ret = iox2_port_factory_subscriber_builder_create(subscriber_builder, NULL, &subscriber);
		if(ret != IOX2_OK)
		{
			printf("iox2_port_factory_subscriber_builder_create is failed: %d\n", ret);
			return -1;
		}

		iox2_service_name_ptr service_name_ptr2 = iox2_cast_service_name_ptr(service_name);
		iox2_service_builder_h service_builder2 = iox2_node_service_builder(&node_handle, NULL, service_name_ptr2);
		iox2_service_builder_event_h service_builder_event = iox2_service_builder_event(service_builder2);

		iox2_port_factory_event_h event_service = NULL;
		ret = iox2_service_builder_event_open_or_create(service_builder_event, NULL, &event_service);
		if(ret != IOX2_OK)
		{
			printf("iox2_service_builder_event_open_or_create is failed: %d\n", ret);
			return -1;
		}
		
		iox2_port_factory_listener_builder_h listener_builder = iox2_port_factory_event_listener_builder(&event_service, NULL);
		ret = iox2_port_factory_listener_builder_create(listener_builder, NULL, &listener);
		if(ret != IOX2_OK)
		{
			printf("iox2_port_factory_listener_builder_create is failed: %d\n", ret);
			return -1;
		}
	}

	pthread_t pthread;
	pthread_create(&pthread, NULL, iox2_thread, NULL);
	pthread_detach(pthread);

    pthread_mutex_init(&mutex, NULL);
	pthread_condattr_init(&condattr);
	pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
	pthread_cond_init(&cond, &condattr);

	sleep(1);

	for(i = 0; i < BYTE_LEN; i++)
	{
		INIT_LIST_HEAD(&time_result[i].node);

		char *buffer = (char *)calloc(byte[i], sizeof(char));
		if(!buffer)
			return -1;
			
		for(int j = 0; j < SAMPLE_LEN; j++)
		{
			time_result[i].send_count++;

			*((unsigned int *)(buffer + 0)) = byte[i];
			*((unsigned int *)(buffer + 4)) = time_result[i].send_count;

			start = get_microsecond();
			iox2_send(buffer, byte[i]);

			struct timespec outtime;
			clock_gettime(CLOCK_MONOTONIC, &outtime);

			unsigned int ms = 1000;
			outtime.tv_sec += ms / 1000;
			unsigned long long us = outtime.tv_nsec / 1000 + ms % 1000 * 1000;
			outtime.tv_sec += us / 1000000;
			us = us % 1000000;
			outtime.tv_nsec = us * 1000;

			int ret = 0;

			pthread_mutex_lock(&mutex);
			while(start_flag == 0 && ret == 0)
				ret = pthread_cond_timedwait(&cond, &mutex, &outtime);
			start_flag = 0;
			pthread_mutex_unlock(&mutex);
		}
	}

	printf("bytes, samples,	send, recv,  stdev,   mean,     min,     50%%,   90%%,   99%%,   99.99%%,   max\n");

	for(i = 0; i < BYTE_LEN; i++)
	{
		bubble_sort_list(&time_result[i].node);

		time_node_t *link = NULL;
		list_for_each_entry(link, &time_result[i].node, list)
			time_result[i].total_time += link->time;

		time_result[i].average_time = time_result[i].total_time / time_result[i].send_count;

		link = NULL;
		list_for_each_entry(link, &time_result[i].node, list)
			time_result[i].standard_deviation_time += pow((link->time - time_result[i].average_time), 2);		
		time_result[i].standard_deviation_time = sqrt(time_result[i].standard_deviation_time / time_result[i].send_count);

		time_result[i].min_time = list_first_entry(&time_result[i].node, time_node_t, list)->time;
		time_result[i].max_time = list_last_entry(&time_result[i].node, time_node_t, list)->time;

		printf("%05d,  %d,   %d, %d, %0.5f, %0.5f, %0.5f, %0.5f\n", byte[i], SAMPLE_LEN, time_result[i].send_count, time_result[i].recv_count, \
													time_result[i].standard_deviation_time, time_result[i].average_time, time_result[i].min_time, \
													time_result[i].max_time);
	}

	return 1;
}
