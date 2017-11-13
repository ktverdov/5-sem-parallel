#ifndef LIST_H
#define LIST_H

#include <stddef.h>
#include <stdlib.h>

typedef struct list_node_t {
	void *data;
	struct list_node_t *next;
} list_node_t;

typedef struct {
	list_node_t *head;
	size_t size;
} list_t;

void NodeFree(list_node_t *node);


void ListInit(list_t *list);
void ListFree(list_t *list);

void Push(list_t *list, void *data);
void Remove(list_t *list, list_node_t *prev_node);
size_t GetSize(list_t *list);

#endif