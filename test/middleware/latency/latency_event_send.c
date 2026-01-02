/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include "list.h"
#include "gateway.h"
#include "event.h"

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

static int event_callback(event_thread_t *event_thread, void *para, unsigned short source_id, unsigned int event, char *buffer, int buffer_len)
{
	if(!event_thread || !buffer)
		goto exit;

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

	return 1;
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

int main(int argc, char *argv[])
{
	gateway_t *gateway = gateway_create(PORT_MPU_TEST_ROUTE, 1);
	gateway_start(gateway);

	event_thread_t *event_thread = event_thread_create(PORT_MPU_TEST_APP0, 10);
	event_thread_attach_event(event_thread, 1234, event_callback, NULL);
	event_thread_start(event_thread);

    pthread_mutex_init(&mutex, NULL);
	pthread_condattr_init(&condattr);
	pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC);
	pthread_cond_init(&cond, &condattr);
	
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
			event_send(event_thread, PORT_MPU_TEST2_APP0, 1235, buffer, byte[i]);

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
