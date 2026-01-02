/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"

typedef struct
{
	unsigned int x;
	struct hlist_node node;
} test_node_t;

int main(int argc, char *argv[])
{
	struct hlist_head hash_head[128];
	
	hash_init(hash_head, 128);
	
	if(!hash_empty(hash_head, 128))
		printf("hash is not empty\n");
	else
		printf("hash is empty\n");
	
	for(int i = 0; i < 1024; i++)
	{
		test_node_t *test_node = (test_node_t *)calloc(1, sizeof(test_node_t));
		test_node->x = i;
		hash_add(hash_head, 128, &test_node->node, test_node->x);
	}
	
	int i = 0;
	test_node_t *link = NULL;
	hash_for_each(hash_head, 128, i, link, node)
		printf("i: %d, link->x: %d\n", i, link->x);
	
	int key = 1023;
	link = NULL;
	int btk = hash_min(key, HASH_BITS(128));
	printf("btk: %d\n", btk);
	hash_for_each_possible(hash_head, 128, link, node, key)
	{
		printf("btk: %d, key: %d, link->x: %d\n", btk, key, link->x);
		if(link->x == key)
			break;
	}
	
	int j = 0;
	link = NULL;
	struct hlist_node *next = NULL;
	hash_for_each_safe(hash_head, 128, j, next, link, node)
	{
		hash_del(&link->node);
		printf("j: %d, link->x: %d\n", j, link->x);
		free(link);
	}

	if(!hash_empty(hash_head, 128))
		printf("hash is not empty\n");
	else
		printf("hash is empty\n");
	
	i = 0;
	link = NULL;
	hash_for_each(hash_head, 128, i, link, node)
		printf("i: %d, link->x: %d\n", i, link->x);
	
	return 1;
}
