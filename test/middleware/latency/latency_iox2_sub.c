/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include "iox2/iceoryx2.h"

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

static double get_microsecond(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);

	return (double)time.tv_sec * 1000 + (double)time.tv_nsec / 1000000;
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
						double start = get_microsecond();

						static unsigned int len = 0, count = 0;
						unsigned int len2 = *((unsigned int *)(packet->frame + 0));
						if(len != len2)
						{
							len = len2;
							count = 0;
						}

						count++;
						*((unsigned int *)(packet->frame + 8)) = count;

						double end = get_microsecond();
						*((float *)(packet->frame + 12)) = (float)(end - start);

						iox2_send(packet->frame, packet->len);
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

	while(1)
		sleep(1);

	return 1;
}
