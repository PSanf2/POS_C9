/*
 * When I initialize tasking I'll need to set up the first task, which
 * will be the kernel.
 * I'll have to set up the process list, and put the process for the kernel on it.
 * 
 */

#include <task.h>

extern u32int kernel_start;
extern u32int kernel_end;

extern volatile page_directory_type *current_page_directory;

static list_type *unused_task_nodes;

static list_type *task_list;
static list_node_type *current_task;

static u32int process_id = 1;

void initialize_tasking()
{
	put_str("\nInitializing tasking.");
	
	// set up the pointer to the list of unused nodes
	unused_task_nodes = (list_type *) 0xFF000000;
	
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
	
	// clear out the stuff on the unused lists
	unused_task_nodes->first = NULL;
	unused_task_nodes->last = NULL;
	
	// get the values for the esp, and ebp
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
	put_str("\nfork called.");
	
	// take care of the allocations and pointers first
	
	// get a pointer to an unused task node
	list_node_type *new_node = get_unused_task_node();
	
	// print it
	put_str("\nnew_node=");
	put_hex((u32int) new_node);
	
	// create a pointer to the node data
	task_type *new_task = (task_type *) new_node->data;
	
	// print it
	put_str("\nnew_task=");
	put_hex((u32int) new_task);
	
	// allocate some space for the page directory structure, above the kernel
	page_directory_type *page_dir = (page_directory_type *) malloc_above(sizeof(page_directory_type), kernel_end, 1);
	
	// print it
	put_str("\npage_dir=");
	put_hex((u32int) page_dir);
	
	// allocate a frame where the new page directory will live
	u32int dir_phys_addr = alloc_frame();
	
	// print it
	put_str("\ndir_phys_addr=");
	put_hex(dir_phys_addr);
	
	// allocate 4KB of page aligned virt addr space for the page directory
	u32int *dir_virt_addr = malloc_above(0x1000, kernel_end, 0x1000);
	
	// print it
	put_str("\ndir_virt_addr=");
	put_hex((u32int) dir_virt_addr);
	
	// map the address
	map_page((u32int) dir_virt_addr, dir_phys_addr);
	
	// put all of the data structures together.
	
	// the virtual and physical address goes on the page directory data structure
	page_dir->virt_addr = dir_virt_addr;
	page_dir->phys_addr = dir_phys_addr;
	
	// the page directory goes on the task
	new_task->page_directory = page_dir;
	
	// print it out
	print_task_node(new_node);
	
	// set up the new page directory
	
	// i'll clear it out first
	memset((u8int *) dir_virt_addr, 0, 4096);
	
	// copy the current page directory into the new place.
	for (u32int i = 0; i < 1024; i++)
	{
		dir_virt_addr[i] = current_page_directory->virt_addr[i];
	}
	
	/*
	// i'll copy the current page directory in to the new one with the
	// exception of the mappings for the virtual memory manager.
	// i have a problem here. if i don't copy the crap for the VMM lists
	// then the new address space won't have any way to allocate virtual
	// address space. if i copy the PDE then the new task will share the
	// VMM with the current task. I don't have a handy way of handling
	// this w/o switching to the new task, and initializing the vmm.
	// it's either that, or i can map some memory into the vmm's list
	// space, and initialize it from here.
	
	// probably better to initialize it from here.
	// save the vmm list mappings from the current page directory,
	// allocate a frame, map it to the vmm area, initialize the vmm,
	// copy the page directory, and restore the original vmm PDE.
	
	// save the current mapping for 1021.
	u32int curr_PD1021 = current_page_directory->virt_addr[1021];
	
	// allocate a frame
	u32int new_vmm_list_phys_addr = alloc_frame();
	
	// map that frame to the address used for the vmm stuff
	map_page(0xFF400000, new_vmm_list_phys_addr);
	
	// initialize the memory manager to create a new virtual address space
	vmm_initialize();
	
	// save the new value on PD[1021] for the new page directory.
	u32int new_PD1021 = current_page_directory->virt_addr[1021];
	
	// restore the original value
	current_page_directory->virt_addr[1021] = curr_PD1021;
	
	// put the new value on the new page directory
	dir_virt_addr[1021] = new_PD1021;
	
	// put the physical address for the new page directory on the proper
	// place in the new page directory
	dir_virt_addr[1023] = dir_phys_addr | 3;
	
	// copy the stack
	// put the node on the list
	*/
	
	put_str("\nfork done.\n");
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
		
		if (highest_virt_addr == NULL)
		{
			highest_virt_addr = highest_node_addr(task_list);
		}
		
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



