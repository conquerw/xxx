/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include "timer2.h"

static void timer_thread(union sigval v)
{
	if(!v.sival_ptr)
		return;
	
	timer2_t *timer2 = (timer2_t *)(v.sival_ptr);
	
	if(timer2->timer2_callback)
		timer2->timer2_callback(timer2->para);
}

int timer2_init(timer2_t *timer2, timer2_callback_t timer2_callback, void *para)
{
	if(!timer2)
		return -1;

	timer2->timer2_callback = timer2_callback;
	timer2->para = para;

	struct sigevent evp;
	memset(&evp, 0x00, sizeof(struct sigevent));

	evp.sigev_value.sival_ptr = timer2;
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = timer_thread;
	
	int ret = timer_create(CLOCK_REALTIME, &evp, &timer2->fd);
	if(ret == -1)
		return -1;

	return 1;
}

int timer2_exit(timer2_t *timer2)
{
	if(!timer2)
		return -1;

	timer_delete(timer2->fd);

	return 1;
}

timer2_t *timer2_create(timer2_callback_t timer2_callback, void *para)
{
	timer2_t *timer2 = (timer2_t *)calloc(1, sizeof(timer2_t));
	if(!timer2)
		return NULL;

	int ret = timer2_init(timer2, timer2_callback, para);
	if(ret == -1)
	{
		free(timer2);
		return NULL;
	}

	return timer2;
}

int timer2_destroy(timer2_t *timer2)
{
	if(!timer2)
		return -1;

	timer2_exit(timer2);
	free(timer2);

	return 1;
}

int timer2_start(timer2_t *timer2, int interval, char is_single)
{
	if(!timer2)
		return -1;

	unsigned int tv_sec = interval / 1000;
	unsigned int tv_nsec = interval % 1000 * 1000 * 1000;
	
	struct itimerspec it;
	memset(&it, 0x00, sizeof(struct itimerspec));
	if(is_single == 0)
	{
		it.it_interval.tv_sec = tv_sec;
		it.it_interval.tv_nsec = tv_nsec;
	}
	else if(is_single == 1)
	{
		it.it_interval.tv_sec = 0;
		it.it_interval.tv_nsec = 0;
	}
    it.it_value.tv_sec = tv_sec;
    it.it_value.tv_nsec = tv_nsec;

	timer_settime(timer2->fd, 0, &it, NULL);

	return 1;
}

int timer2_stop(timer2_t *timer2)
{
	if(!timer2)
		return -1;

	struct itimerspec it;
	memset(&it, 0x00, sizeof(struct itimerspec));
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_nsec = 0;
    it.it_value.tv_sec = 0;
    it.it_value.tv_nsec = 0;
	
	timer_settime(timer2->fd, 0, &it, NULL);
	
	return 1;
}
