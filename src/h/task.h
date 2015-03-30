#ifndef __TASK_H
#define __TASK_H

#include <system.h>

typedef struct page_directory_struct page_directory_type;

extern volatile page_directory_type *current_page_directory;

typedef struct task_struct
{
	u32int id;
	u32int esp;
	u32int ebp;
	volatile page_directory_type *page_directory;
} task_type;

void initialize_tasking();
void print_task_list();
void print_task_node(list_node_type *node);
void print_current_task();
void fork();
void kill(u32int process_id);
list_node_type *get_unused_task_node();

#endif
