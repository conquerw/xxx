/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel.h"
#include "port.h"
#include "middleware_mosquitto.h"

static void middleware_mosquitto_connect_callback(struct mosquitto *mosq, void *obj, int reason_code)
{
	if(!obj || reason_code != 0)
		return;
	
	middleware_mosquitto_t *middleware_mosquitto = (middleware_mosquitto_t *)obj;
	
	mosquitto_subscribe(middleware_mosquitto->mosquitto.mosquitto, NULL, middleware_mosquitto->mosquitto.id, 0);
	mosquitto_subscribe(middleware_mosquitto->mosquitto.mosquitto, NULL, "process65535", 0);
}

int middleware_mosquitto_init(middleware_ops_t *middleware_ops, unsigned short id)
{
	if(!middleware_ops)
		return -1;
	
	middleware_mosquitto_t *middleware_mosquitto = container_of(middleware_ops, middleware_mosquitto_t, middleware_ops);
	if(!middleware_mosquitto)
		return -1;
	
	snprintf(middleware_mosquitto->mosquitto.id, sizeof(middleware_mosquitto->mosquitto.id), "process%05d", id);
	
	mosquitto_lib_init();
	middleware_mosquitto->mosquitto.mosquitto = mosquitto_new(middleware_mosquitto->mosquitto.id, true, middleware_mosquitto);
	mosquitto_connect_callback_set(middleware_mosquitto->mosquitto.mosquitto, middleware_mosquitto_connect_callback);
	
	return 1;
}

int middleware_mosquitto_exit(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;
	
	middleware_mosquitto_t *middleware_mosquitto = container_of(middleware_ops, middleware_mosquitto_t, middleware_ops);
	if(!middleware_mosquitto)
		return -1;
	
	mosquitto_destroy(middleware_mosquitto->mosquitto.mosquitto);
	mosquitto_lib_cleanup();
	
	return 1;
}

int middleware_mosquitto_send(middleware_ops_t *middleware_ops, unsigned short id, sk_buffer_t *sk_buffer)
{
	if(!middleware_ops || id == PORT_UNKOWN || !sk_buffer)
		return -1;
	
	middleware_mosquitto_t *middleware_mosquitto = container_of(middleware_ops, middleware_mosquitto_t, middleware_ops);
	if(!middleware_mosquitto)
		return -1;
	
	char topic[16] = "";
	memset(topic, 0x00, sizeof(topic));
	snprintf(topic, sizeof(topic), "process%05d", id);

	mosquitto_publish(middleware_mosquitto->mosquitto.mosquitto, NULL, topic, sk_buffer->tail - sk_buffer->data, sk_buffer->data, 0, false);
	
	return 1;
}

static void middleware_mosquitto_recv_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	if(!obj || !msg)
		return;
	
	middleware_mosquitto_t *middleware_mosquitto = (middleware_mosquitto_t *)obj;
	
	if(middleware_mosquitto->mosquitto.cb)
		middleware_mosquitto->mosquitto.cb(&middleware_mosquitto->middleware_ops, (char *)msg->payload, msg->payloadlen);
}

int middleware_mosquitto_set_recv_callback(middleware_ops_t *middleware_ops, recv_callback_t cb)
{
	if(!middleware_ops || !cb)
		return -1;
	
	middleware_mosquitto_t *middleware_mosquitto = container_of(middleware_ops, middleware_mosquitto_t, middleware_ops);
	if(!middleware_mosquitto)
		return -1;
	
	middleware_mosquitto->mosquitto.cb = cb;
	mosquitto_message_callback_set(middleware_mosquitto->mosquitto.mosquitto, middleware_mosquitto_recv_callback);
	
	return 1;
}

int middleware_mosquitto_start(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;
	
	middleware_mosquitto_t *middleware_mosquitto = container_of(middleware_ops, middleware_mosquitto_t, middleware_ops);
	if(!middleware_mosquitto)
		return -1;
	
	mosquitto_connect(middleware_mosquitto->mosquitto.mosquitto, "127.0.0.1", 1883, 60);
	mosquitto_loop_start(middleware_mosquitto->mosquitto.mosquitto);
	
	return 1;
}

int middleware_mosquitto_stop(middleware_ops_t *middleware_ops)
{
	if(!middleware_ops)
		return -1;
	
	middleware_mosquitto_t *middleware_mosquitto = container_of(middleware_ops, middleware_mosquitto_t, middleware_ops);
	if(!middleware_mosquitto)
		return -1;
	
	mosquitto_disconnect(middleware_mosquitto->mosquitto.mosquitto);
	mosquitto_loop_stop(middleware_mosquitto->mosquitto.mosquitto, false);
	
	return 1;
}
