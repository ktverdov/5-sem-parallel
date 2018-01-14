#include "list.h"

void NodeFree(list_node_t *node) {
	free(node->data);
	free(node);
}

void ListInit(list_t *list) {
	list->head = NULL;
	list->size = 0;
}

void ListFree(list_t *list) {
	if (list->head == NULL)
		return;

	list_node_t *list_node_to_remove = NULL;
	while ((list->head)->next != NULL) {
		list_node_to_remove = list->head;
		list->head = (list->head)->next;
		NodeFree(list_node_to_remove);
	}

	NodeFree(list->head);
}

void Push(list_t *list, void *data) {
	list_node_t *new_node = (list_node_t *)malloc(sizeof(list_node_t));
	new_node->data = data;
	new_node->next = list->head;

	list->head = new_node;
	list->size++;
}

void Remove(list_t *list, list_node_t *prev_node) {
	list_node_t *list_node_to_remove;

	if (prev_node == NULL) {
		list_node_to_remove = list->head;
		list->head = list_node_to_remove->next;
	} else {
		list_node_to_remove = prev_node->next;
		prev_node->next = list_node_to_remove->next;
	}

	NodeFree(list_node_to_remove);
	list->size--;
}

size_t GetSize(list_t *list) {
	return list->size;
}