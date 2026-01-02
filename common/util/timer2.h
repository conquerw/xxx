// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _TIMER2_H_
#define _TIMER2_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <time.h>

typedef int (*timer2_callback_t) (void *para);

typedef struct
{
	timer_t fd;
	timer2_callback_t timer2_callback;
	void *para;
} timer2_t;

int timer2_init(timer2_t *timer2, timer2_callback_t timer2_callback, void *para);

int timer2_exit(timer2_t *timer2);

timer2_t *timer2_create(timer2_callback_t timer2_callback, void *para);

int timer2_destroy(timer2_t *timer2);

int timer2_start(timer2_t *timer2, int interval, char is_single);

int timer2_stop(timer2_t *timer2);

#ifdef __cplusplus
}
#endif

#endif
