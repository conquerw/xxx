// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _PORT_H_
#define _PORT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "queue.h"
#include "sk_buffer.h"

#define PORT_NUM(x ,y, z) ((((x) & 0x1) << 15) | (((y) & 0xFF) << 7) | (((z) & 0x7F)))
#define PORT_X(id) (((id) & 0x8000) >> 15)
#define PORT_Y(id) (((id) & 0x7F80) >> 7)
#define PORT_Z(id) ((id) & 0x007F)
#define PORT_ROUTE(id) (((id) & 0xFF80) + 1)

enum
{
    PORT_UNKOWN = 0,
    
    PORT_MCU_ROUTE= PORT_NUM(0, 0 ,1),
    PORT_MCU_TEST_ROUTE = PORT_NUM(0, 1, 1),
    PORT_MCU_TEST_APP0,
    PORT_MCU_TEST_APP1,
    PORT_MCU_TEST2_ROUTE = PORT_NUM(0, 2, 1),
    PORT_MCU_TEST2_APP0,
    PORT_MCU_TEST2_APP1,

    PORT_MPU_ROUTE = PORT_NUM(1, 0, 1),
    PORT_CHIP_ROUTE,
    PORT_MPU_TEST_ROUTE = PORT_NUM(1, 1, 1),
    PORT_MPU_TEST_APP0,
    PORT_MPU_TEST2_ROUTE = PORT_NUM(1, 2, 1),
    PORT_MPU_TEST2_APP0,
    PORT_MPU_TEST3_ROUTE = PORT_NUM(1, 3, 1),
    PORT_MPU_TEST3_APP0,
    PORT_MPU_TEST3_APP1,

    PORT_BROADCAST = 0xFFFF,	
};

enum
{
	PORT_STATE_UNKOWN = 0,
	PORT_STATE_EXIT,
    PORT_STATE_INIT,
    PORT_STATE_CONN,
    PORT_STATE_DISCONN,
};

typedef struct port port_t;

typedef int (*broadcast_callback_t)(port_t *port, sk_buffer_t *sk_buffer);

struct port
{
	unsigned short id;
	char state;
	queue_t queue;
	broadcast_callback_t cb;
	void *p;
	struct list_head node;
};

int port_init(port_t *port, unsigned short id);

int port_exit(port_t *port);

port_t *port_create(unsigned short id);

int port_destroy(port_t *port);

port_t *port_get_by_id(unsigned short id);

static inline int port_set_state(port_t *port, char state)
{
	if(!port)
		return -1;
	
	port->state = state;
	
	return 1;
}

static inline char port_get_state(port_t *port)
{
	if(!port)
		return PORT_STATE_UNKOWN;
	
	return port->state;
}

static inline int port_set_broadcast_callback(port_t *port, broadcast_callback_t cb)
{
	if(!port || !cb)
		return -1;
	
	port->cb = cb;
	
	return 1;
}

static inline void port_set_p(port_t *port, void *p)
{
    port->p = p;
}

static inline void *port_get_p(port_t *port)
{
    return port->p;
}

unsigned short port_get_next_jump(unsigned short current, unsigned short dest);

int port_send(port_t *port, unsigned short dest, sk_buffer_t *sk_buffer);

int port_broadcast(port_t *port, sk_buffer_t *sk_buffer);

sk_buffer_t *port_recv(port_t *port);

#ifdef __cplusplus
}
#endif

#endif
