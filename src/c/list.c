#include <list.h>

void insert_after(list_type *list, list_node_type *node, list_node_type *new_node)
{
	new_node->prev = node;
	new_node->next = node->next;
	if (node->next == NULL)
	{
		list->last = new_node;
	}
	else
	{
		node->next->prev = new_node;
	}
	node->next = new_node;
}

void insert_before(list_type *list, list_node_type *node, list_node_type *new_node)
{
	new_node->prev = node->prev;
	new_node->next = node;
	if (node->prev == NULL)
	{
		list->first = new_node;
	}
	else
	{
		node->prev->next = new_node;
	}
	node->prev = new_node;
}

void insert_first(list_type *list, list_node_type *new_node)
{
	if (list->first == NULL)
	{
		list->first = new_node;
		list->last = new_node;
		new_node->prev = NULL;
		new_node->next = NULL;
	}
	else
	{
		insert_before(list, list->first, new_node);
	}
}

void insert_last(list_type *list, list_node_type *new_node)
{
	if (list->last == NULL)
	{
		insert_first(list, new_node);
	}
	else
	{
		insert_after(list, list->last, new_node);
	}
}

void remove(list_type *list, list_node_type *node)
{
	if (node->prev == NULL)
	{
		list->first = node->next;
	}
	else
	{
		node->prev->next = node->next;
	}
	
	if (node->next == NULL)
	{
		list->last = node->prev;
	}
	else
	{
		node->next->prev = node->prev;
	}
}

void print_node(list_node_type *node)
{
	put_str("\nprev=");
	put_hex((u32int) node->prev);
	
	put_str(" node=");
	put_hex((u32int) node);
	
	put_str(" next=");
	put_hex((u32int) node->next);
	
	put_str(" data=");
	put_hex((u32int) node->data);
}

void print_list(list_type *list)
{
	put_str("\nlist=");
	put_hex((u32int) list);
	
	put_str(" first=");
	put_hex((u32int) list->first);
	
	put_str(" last=");
	put_hex((u32int) list->last);
	
	list_node_type *current = list->first;
	do
	{
		print_node(current);
		current = current->next;
	} while (current != NULL);
}












