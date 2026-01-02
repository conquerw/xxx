/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include "list.h"

typedef struct
{
	int x;
	struct list_head node;
} test_node_t;

int main(int argc, char *argv[])
{
	struct list_head node;
	
	INIT_LIST_HEAD(&node);
		
	if(!list_empty(&node))
		printf("list is not empty\n");
	else
		printf("list is empty\n");
	
	test_node_t *link = NULL;
	list_for_each_entry(link, &node, node)
		printf("link->x: %d\n", link->x);

	for(int i = 0; i < 10; i++)
	{
		test_node_t *test_node = (test_node_t *)calloc(1, sizeof(test_node_t));
		test_node->x = i * 10;
		printf("test_node->x: %d\n", test_node->x);
		list_add_tail(&test_node->node, &node);
	}

	if(!list_empty(&node))
		printf("list is not empty\n");
	else
		printf("list is empty\n");

	link = NULL;
	list_for_each_entry(link, &node, node)
		printf("link->x: %d\n", link->x);

	link = NULL;
	test_node_t *next = NULL;
	list_for_each_entry_safe(link, next, &node, node)
	{
		list_del(&link->node);
		printf("link->x: %d\n", link->x);
		free(link);
    }

	if(!list_empty(&node))
		printf("list is not empty\n");
	else
		printf("list is empty\n");
	
	link = NULL;
	list_for_each_entry(link, &node, node)
		printf("link->x: %d\n", link->x);

	return 1;
}
