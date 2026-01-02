/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "timer2.h"

static int hello(void *para)
{
	printf("hello hello hello\n");

	return 1;
}

static int world(void *para)
{
	printf("world world world\n");

	return 1;
}

int main(int argc, char *argv[])
{
	timer2_t *timer2 = timer2_create(hello, NULL);
	timer2_start(timer2, 1005, 0);

	timer2_t *timer3 = timer2_create(world, NULL);
	timer2_start(timer3, 2010, 0);

	while(1)
		sleep(1);

	return 1;
}
