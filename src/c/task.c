/*
 * When I initialize tasking I'll need to set up the first task, which
 * will be the kernel.
 * I'll have to set up the process list, and put the process for the kernel on it.
 * 
 */

#include <task.h>

static list_type *unused_task_nodes;

static list_type *task_list;
static list_node_type *current_task;

static u32int process_id = 1;

void initialize_tasking()
{
	put_str("\nInitializing tasking.");
	
	// set up the pointer to the list of unused nodes
	unused_task_nodes = (list_type *) 0xFF400000;
	
	// set up the pointer to the task list
	task_list = (list_type *) ((u32int *) unused_task_nodes + sizeof(list_type));
	
	// figure out the address to the first node
	u32int node_addr = (u32int) task_list + sizeof(list_type);
	
	// create a pointer to the node
	list_node_type *node = (list_node_type *) node_addr;
	
	// figrue out the address for the node data
	u32int data_addr = (u32int) node + sizeof(list_node_type);
	
	// create a pointer to it
	task_type *data = (task_type *) data_addr;
	
	// clear out the stuff on the list
	task_list->first = NULL;
	task_list->last = NULL;
	
	// clear out the stuff on the node
	node->prev = NULL;
	node->data = data;
	node->next = NULL;
	
	// get the values for the esp, ebp, and eip
	u32int esp;
	u32int ebp;
	
	asm volatile("mov %%esp, %0" : "=r"(esp));
	asm volatile("mov %%ebp, %0" : "=r"(ebp));
	
	// clear the stuff on the data
	data->id = process_id++;
	data->esp = esp;
	data->ebp = ebp;
	data->page_directory = current_page_directory;
	
	// put the node on the list
	insert_first(task_list, node);
	
	// set the current task to be the first one on the list
	current_task = task_list->first;
	
	put_str("\nInitializing tasking done.\n");
}

void print_task_list()
{
	put_str("\ntask_list=");
	put_hex((u32int) task_list);
	
	put_str(" first=");
	put_hex((u32int) task_list->first);
	
	put_str(" last=");
	put_hex((u32int) task_list->last);
	
	list_node_type *current = task_list->first;
	do
	{
		print_task_node(current);
		current = current->next;
	} while (current != NULL);
}

void print_task_node(list_node_type *node)
{
	print_node(node);
	
	task_type *node_data = (task_type *) node->data;
	
	put_str("\n\tid=");
	put_hex(node_data->id);
	
	put_str(" esp=");
	put_hex(node_data->esp);
	
	put_str(" ebp=");
	put_hex(node_data->ebp);
	
	put_str(" page_directory=");
	put_hex((u32int) node_data->page_directory);
	
	print_page_directory(node_data->page_directory);
}

void print_current_task()
{
	print_task_node(current_task);
}

void fork()
{
	
}

void kill(__attribute__((unused)) u32int process_id)
{
	
}

list_node_type *get_unused_task_node()
{
	list_node_type *result = NULL;
	
	// if there's a node available on the unused task nodes list
	if (unused_task_nodes->first != NULL)
	{
		result = unused_task_nodes->first;
		
		remove(unused_task_nodes, unused_task_nodes->first);
	}
	else
	{
		u32int highest_virt_addr = highest_node_addr(unused_task_nodes);
		
		u32int new_node_addr = highest_virt_addr + sizeof(list_type) + sizeof(list_node_type);
		
		u32int new_data_addr = new_node_addr + sizeof(list_node_type);
		
		result = (list_node_type *) new_node_addr;
		
		result->prev = NULL;
		result->data = (u32int *) new_data_addr;
		result->next = NULL;
		
		task_type *result_data = (task_type *) new_data_addr;
		
		result_data->id = NULL;
		result_data->esp = NULL;
		result_data->ebp = NULL;
		result_data->page_directory = NULL;
	}
	
	return result;
}



