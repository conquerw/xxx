/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"
#include "rbtree_augmented.h"
#include "map.h"

typedef struct
{
	int x;
	map_node node;
} test_node_t;

int main(int argc, char *argv[])
{
	map_root test_root = RB_ROOT;
	
	if(!map_empty(&test_root))
		printf("map is not empty\n");
	else
		printf("map is empty\n");
	
    test_node_t *link = NULL;
	map_for_each_entry(link, &test_root, node)
		printf("key: %llu, x: %d\n", link->node.key, link->x);
	
	for(int i = 5; i < 10; i++)
	{
		test_node_t *test_node = (test_node_t *)calloc(1, sizeof(test_node_t));
		test_node->x = i * 10;
		test_node->node.key = test_node->x;
		printf("add node: %llu\n", test_node->node.key);
		map_add(&test_root, &test_node->node);
		
		if(i == 7)
		{
			test_node_t *test_node = (test_node_t *)calloc(1, sizeof(test_node_t));
			test_node->x = 1 * 10;
			test_node->node.key = test_node->x;
			printf("add node: %llu\n", test_node->node.key);
			map_add(&test_root, &test_node->node);
		}
		
		test_node = map_first_entry(&test_root, test_node_t, node);
		if(test_node != NULL)
			printf("test_node->node.key: %llu\n", test_node->node.key);
	}
	
    link = NULL;
	map_for_each_entry(link, &test_root, node)
		printf("key: %llu, x: %d\n", link->node.key, link->x);

	link = NULL;
	test_node_t *next = NULL;
	map_for_each_entry_safe(link, next, &test_root, node)
	{
        map_remove(&test_root, link->node.key);
        printf("delete node: %llu\n", link->node.key);
		free(link);
    }
	
	if(!map_empty(&test_root))
		printf("map is not empty\n");
	else
		printf("map is empty\n");
	
    link = NULL;
	map_for_each_entry(link, &test_root, node)
		printf("key: %llu, x: %d\n", link->node.key, link->x);
	
	return 1;
}
