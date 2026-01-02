/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdlib.h>
#include <string.h>
#include "kernel.h"
#include "port.h"
#include "middleware.h"

#if defined(MIDDLEWARE_MOSQUITTO)
#include "middleware_mosquitto.h"
#endif
#if defined(MIDDLEWARE_IOX2)
#include "middleware_iox2.h"
#endif
#if defined(MIDDLEWARE_NNG)
#include "middleware_nng.h"
#endif

middleware_ops_t *middleware_create(char type)
{
	if(type == MIDDLEWARE_TYPE_UNKOWN)
		return NULL;

	#if defined(MIDDLEWARE_MOSQUITTO)
	if(type == MIDDLEWARE_TYPE_MOSQUITTO)
	{
		middleware_mosquitto_t *middleware_mosquitto = (middleware_mosquitto_t *)calloc(1, sizeof(middleware_mosquitto_t));
		if(!middleware_mosquitto)
			return NULL;
		
		middleware_mosquitto->middleware_ops.init = middleware_mosquitto_init;
		middleware_mosquitto->middleware_ops.exit = middleware_mosquitto_exit;
		middleware_mosquitto->middleware_ops.send = middleware_mosquitto_send;
		middleware_mosquitto->middleware_ops.set_recv_callback = middleware_mosquitto_set_recv_callback;
		middleware_mosquitto->middleware_ops.start = middleware_mosquitto_start;
		middleware_mosquitto->middleware_ops.stop = middleware_mosquitto_stop;
		
		return &middleware_mosquitto->middleware_ops;
	}
	#endif

	#if defined(MIDDLEWARE_IOX2)
	if(type == MIDDLEWARE_TYPE_IOX2)
	{
		middleware_iox2_t *middleware_iox2 = (middleware_iox2_t *)calloc(1, sizeof(middleware_iox2_t));
		if(!middleware_iox2)
			return NULL;
		
		middleware_iox2->middleware_ops.init = middleware_iox2_init;
		middleware_iox2->middleware_ops.exit = middleware_iox2_exit;
		middleware_iox2->middleware_ops.send = middleware_iox2_send;
		middleware_iox2->middleware_ops.set_recv_callback = middleware_iox2_set_recv_callback;
		middleware_iox2->middleware_ops.start = middleware_iox2_start;
		middleware_iox2->middleware_ops.stop = middleware_iox2_stop;
		
		return &middleware_iox2->middleware_ops;
	}
	#endif

	#if defined(MIDDLEWARE_NNG)
	if(type == MIDDLEWARE_TYPE_NNG)
	{
		middleware_nng_t *middleware_nng = (middleware_nng_t *)calloc(1, sizeof(middleware_nng_t));
		if(!middleware_nng)
			return NULL;
		
		middleware_nng->middleware_ops.init = middleware_nng_init;
		middleware_nng->middleware_ops.exit = middleware_nng_exit;
		middleware_nng->middleware_ops.send = middleware_nng_send;
		middleware_nng->middleware_ops.set_recv_callback = middleware_nng_set_recv_callback;
		middleware_nng->middleware_ops.start = middleware_nng_start;
		middleware_nng->middleware_ops.stop = middleware_nng_stop;
		
		return &middleware_nng->middleware_ops;
	}
	#endif

	return NULL;
}

int middleware_destroy(middleware_ops_t *middleware_ops, char type)
{
	if(!middleware_ops || type == MIDDLEWARE_TYPE_UNKOWN)
		return -1;

	#if defined(MIDDLEWARE_MOSQUITTO)
	if(type == MIDDLEWARE_TYPE_MOSQUITTO)
	{
		middleware_mosquitto_t *middleware_mosquitto = container_of(middleware_ops, middleware_mosquitto_t, middleware_ops);
		if(!middleware_mosquitto)
			return -1;
		
		free(middleware_mosquitto);
	}
	#endif

	#if defined(MIDDLEWARE_IOX2)
	if(type == MIDDLEWARE_TYPE_IOX2)
	{
		middleware_iox2_t *middleware_iox2 = container_of(middleware_ops, middleware_iox2_t, middleware_ops);
		if(!middleware_iox2)
			return -1;
		
		free(middleware_iox2);
	}
	#endif

	#if defined(MIDDLEWARE_NNG)
	if(type == MIDDLEWARE_TYPE_NNG)
	{
		middleware_nng_t *middleware_nng = container_of(middleware_ops, middleware_nng_t, middleware_ops);
		if(!middleware_nng)
			return -1;
		
		free(middleware_nng);
	}
	#endif

	return 1;
}
