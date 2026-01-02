/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <pthread.h>
#include "queue.h"

typedef struct
{
	int value;
	struct list_head node;
} test_t;

static atomic_int count;
static double start_time = 0;
static double end_time = 0;

static queue_t queue;

static double get_millisecond(void)
{
    struct timeval current_time;
	
    gettimeofday(&current_time, NULL);
	
	double time = (double)current_time.tv_sec * 1000 + (double)current_time.tv_usec / 1000;
	
	return time;
}

static void *push_thread(void *para)
{
	if(!para)
		return NULL;
	
	int i = *((int *)para) + 1;
	
	pid_t pid = syscall(SYS_gettid);
	
	while(1)
	{
		test_t *test = (test_t *)calloc(1, sizeof(test_t));
		if(!test)
			break;
		
		test->value = i;	
		queue_push(&queue, &test->node);
		
		atomic_fetch_add(&count, 1);
		
		printf("push_thread: %d, %d, %d\n", pid, test->value, count);
		
		if(count >= 1000)
			break;
		
		usleep(10 * 1000);
	}
	
	return NULL;
}

static void *pop_thread(void *para)
{
	sleep(3);
	
	pid_t pid = syscall(SYS_gettid);
	
	while(1)
	{
		struct list_head *node = queue_pop(&queue);
		if(node)
		{
			test_t *test = list_entry(node, test_t, node);
			if(test)
				printf("pop_thread : %d, %d, %d\n", pid, test->value, count);
			
			if(count >= 1000)
			{
				end_time = get_millisecond();
				break;
			}
		}
	}
	
	return NULL;
}

int main(int argc, char *argv[])
{
	queue_init(&queue, -1);
	atomic_init(&count, 0);
	start_time = get_millisecond();
	
	int i = 0;
	pthread_t id[6];
	
	for(i = 0; i < 4; i++)
		pthread_create(&id[i], NULL, push_thread, (void *)&i);
	
	for(i = 4; i < 6; i++)
		pthread_create(&id[i], NULL, pop_thread, NULL);
	
	while(1)
	{
		if(end_time != 0)
		{
			printf("start_time, end_time, total time: %f, %f, %f\n", start_time, end_time, end_time - start_time);
			break;
		}
		sleep(1);
	}
}
