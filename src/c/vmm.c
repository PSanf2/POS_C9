#include <vmm.h>

extern u32int page_table_virt_addr;

list_type *vmm_unused_nodes;
list_type *vmm_used;
list_type *vmm_free;

void vmm_initialize()
{
	/*
	 * I need to determine the virtual address of the kernel's page table
	 * and put the list right after that. Going over the page directory
	 * is useless for this because of it's 4 KB granularity, and the fact
	 * that 4 MB of pages are mapped when the kernel is loaded. I have
	 * to do this without causing any page faults. This means that I'll
	 * need to compact the list after each node is created in order to
	 * keep my memory usage to an absolute minimum. This is because stuff
	 * in the paging code (specifically the page fault interrupt handler)
	 * will rely on code in the virtual memory manager. Because I have
	 * a full 4 MB of memory mapped this means that I won't need to
	 * actually evaluate every page table for 4 KB portions of free space.
	 * In fact, since I already know the state of the memory allocations
	 * coming in to this function it might just be better to create a few
	 * nodes for all of the space I know is free. I'll know that everything
	 * from 0x0 - BFFF FFFF is free, everything from 0xC000 0000 - 0xC040 0000
	 * is used, and everything from 0xC040 0001 - 0xFFFF FFFF is free.
	 * Since the stuff in that first 4 MB of space is vital to kernel
	 * operations it'll never be available to be freed, and doesn't need
	 * to go on any list. The same is true for the 4 MB of virtual
	 * address space that's used by the recursive mappings.
	 */
	
	// start below here to keep the indenting neat
	u32int vmm_starting_addr = page_table_virt_addr + 0x1000;
	
	/*
	put_str("\nvmm_starting_addr=");
	put_hex(vmm_starting_addr);
	put_str("\nsizeof(list_type)=");
	put_hex(sizeof(list_type));
	*/
	
	// set up the pointers for the lists
	vmm_unused_nodes = (list_type *) vmm_starting_addr;
	vmm_used = (list_type *) ((u32int) vmm_unused_nodes + sizeof(list_type));
	vmm_free = (list_type *) ((u32int) vmm_used + sizeof(list_type));
	
	/*
	// print it
	put_str("\nvmm_unused_nodes=");
	put_hex((u32int) vmm_unused_nodes);
	put_str("\nvmm_used=");
	put_hex((u32int) vmm_used);
	put_str("\nvmm_free=");
	put_hex((u32int) vmm_free);
	*/
	
	// clear the lists
	vmm_unused_nodes->first = NULL;
	vmm_unused_nodes->last = NULL;
	
	vmm_used->first = NULL;
	vmm_used->last = NULL;
	
	vmm_free->first = NULL;
	vmm_free->last = NULL;
	
	// set up the pointers for the first node
	list_node_type *node = (list_node_type *) ((u32int) vmm_free + sizeof(list_type));
	vmm_data_type *data = (vmm_data_type *) ((u32int) node + sizeof(list_node_type));
	
	/*
	// print it
	put_str("\nsizeof(list_type)=");
	put_hex(sizeof(list_type));
	put_str("\nnode=");
	put_hex((u32int) node);
	put_str("\nsizeof(list_node_type)=");
	put_hex(sizeof(list_node_type));
	put_str("\ndata=");
	put_hex((u32int) data);
	*/
	
	// populate the node
	node->prev = NULL;
	node->data = data;
	node->next = NULL;
	
	// populate the data
	data->virt_addr = 0x0;
	data->size = 0xC0000000;
	
	// put the node on the list
	insert_first(vmm_free, node);
	
	// figure out the address for the next node on the list
	node = (list_node_type *) ((u32int) data + sizeof(vmm_data_type));
	data = (vmm_data_type *) ((u32int) node + sizeof(list_node_type));
	
	/*
	// print it
	put_str("\nsizeof(vmm_data_type)=");
	put_hex(sizeof(vmm_data_type));
	put_str("\nnode=");
	put_hex((u32int) node);
	put_str("\ndata=");
	put_hex((u32int) data);
	*/
	
	// populate the new node
	node->prev = NULL;
	node->data = data;
	node->next = NULL;
	
	// populate the data
	data->virt_addr = 0xC0400000;
	data->size = 0x3F800000;
	
	// put it on the list
	insert_last(vmm_free, node);
	
	/*
	// print the list
	vmm_print_list(vmm_free);
	put_str("\n");
	*/
	
}

u32int *malloc(u32int size)
{
	return malloc_align(size, 0x1);
}

u32int *malloc_align(u32int size, u32int align)
{
	return malloc_above(size, align, 0x0);
}

u32int *malloc_above(u32int size, u32int align, u32int above)
{
	list_node_type *malloc_node = search_free((size + align), above);
	
	vmm_data_type *malloc_data = malloc_node->data;
	
	u32int start_addr = malloc_data->virt_addr;
	u32int split_size = 0;
	
	while (start_addr < above)
	{
		start_addr++;
		split_size++;
	}
	
	while (start_addr % align != 0)
	{
		start_addr++;
		split_size++;
	}
	
	if (split_size > 0)
	{
		malloc_node = split_free(malloc_node, size);
		malloc_node = malloc_node->next;
	}
	
	list_node_type *split_node = split_free(malloc_node, size);
	
	remove(vmm_free, split_node);
	
	insert_last(vmm_used, split_node);
	
	vmm_data_type *malloc_node_data = (vmm_data_type *) malloc_node->data;
	u32int *malloc_ptr = (u32int *) malloc_node_data->virt_addr;
	
	return malloc_ptr;
}

void free(u32int *virt_addr)
{
	list_node_type *used_node = search_used(virt_addr);
	list_node_type *goes_before = search_free_neighbor(used_node);
	
	remove(vmm_used, used_node);
	
	insert_before(vmm_free, goes_before, used_node);
	
	compact_all_free();
	
}

void vmm_print_node(list_node_type *node)
{
	print_node(node);
	
	vmm_data_type *node_data = (vmm_data_type *) node->data;
	
	put_str("\n\tnode_data=");
	put_hex((u32int) node_data);
	
	put_str(" virt_addr=");
	put_hex(node_data->virt_addr);
	
	put_str(" size=");
	put_hex(node_data->size);
}

void vmm_print_list(list_type *list)
{
	put_str("\nlist=");
	put_hex((u32int) list);
	
	put_str(" first=");
	put_hex((u32int) list->first);
	
	put_str(" last=");
	put_hex((u32int) list->last);
	
	if (list->first != NULL)
	{
		list_node_type *current = list->first;
		do
		{
			vmm_print_node(current);
			current = current->next;
		} while (current != NULL);
	}
	else
	{
		put_str("\nEmpty list.");
	}
	put_str("\n");
}

void vmm_print_free()
{
	vmm_print_list(vmm_free);
}

void vmm_print_used()
{
	vmm_print_list(vmm_used);
}

list_node_type *split_free(list_node_type *node, u32int size)
{
	vmm_data_type *node_data = node->data;
	list_node_type *new_node = get_unused_node();
	vmm_data_type *new_node_data = new_node->data;
	new_node_data->virt_addr = node_data->virt_addr + size;
	new_node_data->size = node_data->size - size;
	node_data->size = size;
	insert_after(vmm_free, node, new_node);
	return node;
}

list_node_type *search_free(u32int size, u32int above)
{
	list_node_type *result = NULL;
	list_node_type *candidate = vmm_free->first;
	
	do
	{
		
		vmm_data_type *node_data = candidate->data;
		
		if ((((node_data->virt_addr <= above) && (node_data->virt_addr + node_data->size > above)) || (node_data->virt_addr >= above)) && (node_data->size >= size))
		{
			result = candidate;
			break;
		}
		
		candidate = candidate->next;
	} while (candidate != NULL);
	
	
	return result;
}

list_node_type *get_unused_node()
{
	list_node_type *result = NULL;
	
	if (vmm_unused_nodes->first != NULL)
	{
		result = vmm_unused_nodes->first;
		remove(vmm_unused_nodes, vmm_unused_nodes->first);
	}
	else
	{
		u32int highest_node_addr = (u32int) highest_node(vmm_free);
		
		if (((u32int) highest_node(vmm_used)) > highest_node_addr)
		{
			highest_node_addr = (u32int) highest_node(vmm_used);
		}
		
		u32int new_node_addr = highest_node_addr + sizeof(list_type) + sizeof(list_node_type);
		
		u32int new_data_addr = new_node_addr + sizeof(list_node_type);
		
		result = (list_node_type *) new_node_addr;
		
		result->prev = NULL;
		result->data = (u32int *) new_data_addr;
		result->next = NULL;
		
		vmm_data_type *result_data = (vmm_data_type *) new_data_addr;
		
		result_data->virt_addr = NULL;
		result_data->size = NULL;
	}
	
	return result;
}

list_node_type *search_used(u32int *virt_addr)
{
	list_node_type *result = NULL;
	list_node_type *candidate = vmm_used->first;
	
	do
	{
		vmm_data_type *candidate_data = candidate->data;
		
		if (candidate_data->virt_addr == (u32int) virt_addr)
		{
			result = candidate;
			break;
		}
		
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

list_node_type *search_free_neighbor(list_node_type *node)
{
	list_node_type *result = NULL;
	list_node_type *candidate = vmm_free->first;
	
	do
	{
		vmm_data_type *candidate_data = candidate->data;
		vmm_data_type *node_data = node->data;
		
		if (candidate_data->virt_addr > node_data->virt_addr)
		{
			result = candidate;
			break;
		}
		
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

list_node_type *search_adjacent_free()
{
	list_node_type *result = NULL;
	list_node_type *candidate = vmm_free->first;
	
	do
	{
		
		if (candidate->next != NULL)
		{
			vmm_data_type *candidate_data = (vmm_data_type *) candidate->data;
			vmm_data_type *candidate_next_data = (vmm_data_type *) candidate->next->data;
			
			if ((candidate_data->virt_addr + candidate_data->size) == candidate_next_data->virt_addr)
			{
				result = candidate;
				break;
			}
		}
		
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

void compact_after(list_node_type *node)
{
	vmm_data_type *node_data = (vmm_data_type *) node->data;
	
	list_node_type *next_node = node->next;
	vmm_data_type *next_node_data = (vmm_data_type *) next_node->data;
	
	u32int new_size = node_data->size + next_node_data->size;
	
	node_data->size = new_size;
	
	remove(vmm_free, next_node);
	
	insert_last(vmm_unused_nodes, next_node);
}

void compact_all_free()
{
	list_node_type *node = search_adjacent_free();
	
	while (node != NULL)
	{
		compact_after(node);
		node = search_adjacent_free();
	}
}
