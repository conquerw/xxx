// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _SK_BUFFER_H_
#define _SK_BUFFER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

#define HEADROOM_SIZE 32
#define TAILROOM_SIZE 0

typedef struct
{
    char *head;
    char *data;
    char *tail;
    char *end;
	struct list_head node;
	char buffer[0];
} sk_buffer_t;

static inline sk_buffer_t *sk_buffer_create(int head_len, int len, int tail_len)
{
    int len2 = head_len + len + tail_len;
    sk_buffer_t *sk_buffer = (sk_buffer_t *)calloc(1, sizeof(sk_buffer_t) + len2);
    if(!sk_buffer)
        return NULL;

    sk_buffer->head = sk_buffer->buffer;
    sk_buffer->end = sk_buffer->buffer + len2;
    sk_buffer->data = sk_buffer->buffer + head_len;
    sk_buffer->tail = sk_buffer->buffer + len2 - tail_len;

    return sk_buffer;
}

static inline int sk_buffer_destroy(sk_buffer_t *sk_buffer)
{
    if(!sk_buffer)
        return -1;

    free(sk_buffer);

    return 1;
}

static inline int sk_buffer_data_copy(sk_buffer_t *sk_buffer, const char *buffer, int buffer_len)
{
    if(!sk_buffer || !buffer || buffer_len == 0)
        return -1;

    if(sk_buffer->data + buffer_len > sk_buffer->tail)
    {
        printf("%s - %d space is not enough\n", __func__, __LINE__);
        return -1;
    }
	
    memcpy(sk_buffer->data, buffer, buffer_len);

    return 1;
}

static inline int sk_buffer_push_copy(sk_buffer_t *sk_buffer, const char *buffer, int buffer_len)
{
    if(!sk_buffer || !buffer || buffer_len == 0)
        return -1;

    if(sk_buffer->data - sk_buffer->head < buffer_len)
    {
        printf("%s - %d space is not enough\n", __func__, __LINE__);
        return -1;
    }
	
    sk_buffer->data -= buffer_len;
    memcpy(sk_buffer->data, buffer, buffer_len);
	
    return 1;
}

static inline int sk_buffer_pull(sk_buffer_t *sk_buffer, int len)
{
    if(!sk_buffer || len == 0)
        return -1;

    if(sk_buffer->data + len > sk_buffer->tail)
    {
        printf("%s - %d space is not enough\n", __func__, __LINE__);
        return -1;
    }

    sk_buffer->data += len;
	
    return 1;
}

#ifdef __cplusplus
}
#endif

#endif
