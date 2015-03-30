#ifndef __LIST_H
#define __LIST_H

#include <system.h>

typedef struct list_node_struct
{
	struct list_node_struct *prev;
	void *data;
	struct list_node_struct *next;
} list_node_type;

typedef struct list_struct
{
	struct list_node_struct *first;
	struct list_node_struct *last;
} list_type;

void insert_after(list_type *list, list_node_type *node, list_node_type *new_node);
void insert_before(list_type *list, list_node_type *node, list_node_type *new_node);
void insert_first(list_type *list, list_node_type *new_node);
void insert_last(list_type *list, list_node_type *new_node);
void remove(list_type *list, list_node_type *node);
void print_node(list_node_type *node);
void print_list(list_type *list);
u32int highest_node_addr(list_type *list);

#endif
