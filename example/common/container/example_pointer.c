/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
	char *name;
	int *age;
	int height;
} person_t;

int main(int argc, char *argv[])
{
	person_t *person = NULL;
	printf("person: %p\n", person);
	
	person = calloc(1, sizeof(person_t));
	printf("person: %p\n", person);
	
	person->name = calloc(16, sizeof(char));
	memcpy(person->name, "China", strlen("China"));
	person->age = calloc(1, sizeof(int));
	*person->age = 28;
	person->height = 175;
	
	printf("person: %p\n", person);
	printf("person.name: %s\n", person->name);
	printf("person.age: %d\n", *person->age);
	printf("person.height: %d\n", person->height);
	
	free(person->name);
	free(person->age);
	free(person);
	
	printf("person: %p\n", person);
	printf("person.name: %s\n", person->name);
	printf("person.age: %d\n", *person->age);
	printf("person.height: %d\n", person->height);
	
	person->name = NULL;
	person->age = NULL;
	person = NULL;
	
	printf("person: %p\n", person);
	printf("person.name: %s\n", person->name);
	printf("person.age: %d\n", *person->age);
	printf("person.height: %d\n", person->height);
	
	return 1;
}
