/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sched.h>
#include "mosquitto.h"
#include "util.h"

static double start = 0, end = 0;
static unsigned char is_end = 0;
static unsigned int count = 0, total_len = 0;

static double get_microsecond(void)
{
	struct timespec time;

	clock_gettime(CLOCK_MONOTONIC, &time);

	return (double)time.tv_sec * 1000 + (double)time.tv_nsec / 1000000;
}

static void connect_callback(struct mosquitto *mosq, void *obj, int reason_code)
{
	if(reason_code != 0)
		return;

	char topic[16] = "";
	memset(topic, 0x00, sizeof(topic));
	snprintf(topic, sizeof(topic), "process%d", 11);
	
	mosquitto_subscribe(mosq, NULL, topic, 0);
}

static void recv_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	if(!mosq || !msg)
		return;

	char *buffer = (char *)msg->payload;

 	if(buffer[0] == 1)
	{
		char topic[16] = "";
		memset(topic, 0x00, sizeof(topic));
		snprintf(topic, sizeof(topic), "process%d", 22);

		char cmd = 1;
		mosquitto_publish(mosq, NULL, topic, 1, &cmd, 0, false);
	}
	if(buffer[0] == 2)
	{
		is_end = count = total_len = 0;
		start = get_microsecond();
	}
	else if(buffer[0] == 3)
	{
		count++;
		total_len += msg->payloadlen;
	}
	else if(buffer[0] == 4)
	{
		end = get_microsecond();
		is_end = 1;
	}
}

int main(int argc, char *argv[])
{
	set_root_caps();
	
    struct sched_param param;
    param.sched_priority = 80;
	sched_setscheduler(0, SCHED_RR, &param);

	mosquitto_lib_init();
	struct mosquitto *mosquitto = mosquitto_new(NULL, true, NULL);
	
	mosquitto_connect_callback_set(mosquitto, connect_callback);
	mosquitto_message_callback_set(mosquitto, recv_callback);

	mosquitto_connect(mosquitto, "127.0.0.1", 1883, 60);
	
	mosquitto_loop_start(mosquitto);

	while(1)
	{
		if(is_end == 1)
		{
			is_end = 0;

			char topic[16] = "";
			memset(topic, 0x00, sizeof(topic));
			snprintf(topic, sizeof(topic), "process%d", 22);

			char buffer[16] = "";
			memset(buffer, 0x00, sizeof(buffer));

			*((unsigned char *)(buffer + 0)) = 5;
			*((unsigned int *)(buffer + 1)) = total_len;
			*((unsigned int *)(buffer + 5)) = count;
			*((float *)(buffer + 9)) = (float)(end - start);

			mosquitto_publish(mosquitto, NULL, topic, sizeof(buffer), buffer, 0, false);

			printf("total_len, count: %d, %d\n", total_len, count);
		}

		sleep(1);
	}

	return 1;
}
