/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "mosquitto.h"

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

	double start = get_microsecond();

	static unsigned int len = 0, count = 0;
	unsigned int len2 = *((unsigned int *)(msg->payload + 0));
	if(len != len2)
	{
		len = len2;
		count = 0;
	}

	count++;
	*((unsigned int *)(msg->payload + 8)) = count;

	char topic[16] = "";
	memset(topic, 0x00, sizeof(topic));
	snprintf(topic, sizeof(topic), "process%d", 22);

	double end = get_microsecond();
	*((float *)(msg->payload + 12)) = (float)(end - start);

	mosquitto_publish(mosq, NULL, topic, msg->payloadlen, msg->payload, 0, false);
}

int main(int argc, char *argv[])
{
	mosquitto_lib_init();
	struct mosquitto *mosquitto = mosquitto_new(NULL, true, NULL);
	
	mosquitto_connect_callback_set(mosquitto, connect_callback);
	mosquitto_message_callback_set(mosquitto, recv_callback);

	mosquitto_connect(mosquitto, "127.0.0.1", 1883, 60);
	
	mosquitto_loop_start(mosquitto);

	while(1)
		sleep(1);

	return 1;
}
