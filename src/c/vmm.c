#include <vmm.h>

// i need to have a free and used memory list
// the used memory list should never be compacted.
// it'll be needed to store information about which memory ranges have been allocated.
// when memory is allocated a free node will be split, the node for the allocated memory
// will be removed from the free list, and then put on the used list.
// freeing memory will be a reverse process where the node is put on the free list, and the list
// will be compacted.

// don't fall in to that trap with the search function causing a page fault trying to read from a null pointer

// whenever i compact two free mem nodes it's resulting in a pointer being lost. this is causing a memory leak.
// when i compact free mem nodes the node that's removed from the free mem list should be put on a "free nodes" list so it can be recycled.
// when ever i split a node i should look on the "free nodes" list to see if there is a free node available before creating a new one
// i should never need to worry about finding the address to use for the free nodes list becuase they've already been placed.
// i'm just making sure i keep them around so i don't loose them, and create a memeory leak

static list_type *unused_nodes;
static list_type *used_memory;
static list_type *free_memory;

void vmm_initialize()
{
	put_str("\nVMM Initialize");
	
	unused_nodes = (list_type *) 0xFF800000;
	used_memory = (list_type *) ((u32int) unused_nodes + sizeof(list_type));
	free_memory = (list_type *) ((u32int) used_memory + sizeof(list_type));
	
	// print it out
	put_str("\nsizeof(list_type)=");
	put_hex(sizeof(list_type));
	
	put_str("\nunused_nodes=");
	put_hex((u32int) unused_nodes);
	
	put_str("\nused_memory=");
	put_hex((u32int) used_memory);
	
	put_str("\nfree_memory=");
	put_hex((u32int) free_memory);
	
	put_str("\nsizeof(list_node_type)=");
	put_hex(sizeof(list_node_type));
	
	// initialize the lists to null pointers
	
	unused_nodes->first = NULL;
	unused_nodes->last = NULL;
	
	used_memory->first = NULL;
	used_memory->last = NULL;
	
	free_memory->first = NULL;
	free_memory->last = NULL;
	
	// rename the pointer to the current page directory so it's easier to work with
	u32int *page_directory = current_page_directory->virt_addr;
	
	/*
	 * The for loop doesn't run all the way up to 1024 because I don't
	 * want malloc to be able to return virtual addresses above
	 * 0xFF40 0000 because addresses above those are reserved for kernel
	 * stuff.
	 * 
	 * 0xFF40 0000 => PD[1021] = mappings for process control blocks (4MB)
	 * 0xFF80 0000 => PD[1022], PT[0] - PT[767] = mappings for the lists used by the virtual memory manager (3MB)
	 * 0xFFB0 0000 => PD[1022], PT[768] = mappings for the bitmap for the physical memory manager. (1MB)
	 * 0xFFC0 0000 => PD[1023] = recursive mappings for page tables (4MB)
	 */
	 
	// for each entry on the page directory
	for (u32int i = 0; i < 1021; i++)
	{
		// if there's nothing on that index of the page directory
		if ((page_directory[i] & 0x1) == 0)
		{
			// figure out the virtual address where that 4MB of memory begins
			u32int free_addr = i * 0x400000;
			
			// figure out the size of the node to add to the list
			u32int free_size = 0x400000;
			
			// figure out the virtual address where the list node will live
			u32int new_node_addr;
			if (free_memory->last == NULL)
			{
				new_node_addr = (u32int) free_memory + sizeof(list_type);
			}
			else
			{
				new_node_addr = (u32int) free_memory->last + sizeof(list_type) + sizeof(list_node_type);
			}
			
			// figure out the virtual address where the node data will live
			u32int new_data_addr = new_node_addr + sizeof(list_node_type);
			
			// make a pointer to the place where the node will live
			list_node_type *new_node = (list_node_type *) new_node_addr;
			
			// make a pointer to the place where the node data will live
			allocation_type *new_data = (allocation_type *) new_data_addr;
			
			// populate the important values on the node
			new_node->data = new_data;
			
			// populate the important values on the node data
			new_data->virt_addr = free_addr;
			new_data->size = free_size;
			
			// add that node to the end of the list
			insert_last(free_memory, new_node);
			
		}
		// else there's a value for that page directory entry, so i need to go over the page table i've found
		else
		{
			// figure out the virtual address for that page table
			u32int virt_addr_base = 0xFFC00000;
			u32int virt_addr_offset = i << 12;
			u32int page_table_virt_addr = virt_addr_base + virt_addr_offset;
			
			// make a pointer to the page table
			u32int *page_table = (u32int *) page_table_virt_addr;
			
			// for each entry on the page table
			for (u32int j = 0; j < 1024; j++)
			{
				// if there's nothing on the page table at that index
				if ((page_table[j] & 0x1) == 0)
				{
					// figure out the virtual address where that 4KB of memory begins
					u32int free_addr = ((i * 0x400000) + (j * 0x1000));
					
					// figure out the size of the node to add to the list
					u32int free_size = 0x1000;
					
					// figure out the virtual address where the node will live
					u32int new_node_addr;
					if (free_memory->last == NULL)
					{
						new_node_addr = (u32int) free_memory + sizeof(list_type);
					}
					else
					{
						new_node_addr = (u32int) free_memory->last + sizeof(list_type) + sizeof(list_node_type);
					}
					
					// figure out the virtual address where the node data will live
					u32int new_data_addr = new_node_addr + sizeof(list_node_type);
					
					// make a pointer to the place where the node will live
					list_node_type *new_node = (list_node_type *) new_node_addr;
					
					// make a pointer to the place where the node data will live
					allocation_type *new_data = (allocation_type *) new_data_addr;
					
					// populate the important values on the node
					new_node->data = new_data;
					
					// populate the important values on the node data
					new_data->virt_addr = free_addr;
					new_data->size = free_size;
					
					// add the node to the end of the list
					insert_last(free_memory, new_node);
					
				}
			}
		}
	}
	
	// i need to compact all of the free nodes
	compact_all_free();
	
	put_str("\nVMM Initialize Done.\n");
}

void print_allocation_node(list_node_type *node)
{
	print_node(node);
	
	allocation_type *node_data = (allocation_type *) node->data;
	
	put_str("\n\tnode_data=");
	put_hex((u32int) node_data);
	
	put_str(" virt_addr=");
	put_hex(node_data->virt_addr);
	
	put_str(" size=");
	put_hex(node_data->size);
}

void print_allocation_list(list_type *list)
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
		print_allocation_node(current);
		current = current->next;
	} while (current != NULL);
}

void compact_after(list_node_type *node)
{
	// compacts a node with the one after it
	allocation_type *node_data = (allocation_type *) node->data;
	
	list_node_type *next_node = node->next;
	allocation_type *next_node_data = (allocation_type *) next_node->data;
	
	u32int new_size = node_data->size + next_node_data->size;
	
	node_data->size = new_size;
	
	remove(free_memory, next_node);
	
	insert_last(unused_nodes, next_node);
	
}

list_node_type *search_adjacent_free()
{
	list_node_type *result = NULL;
	list_node_type *candidate = free_memory->first;
	
	do
	{
		if (candidate->next != NULL)
		{
			allocation_type *candidate_data = (allocation_type *) candidate->data;
			allocation_type *candidate_next_data = (allocation_type *) candidate->next->data;
			
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

void compact_all_free()
{
	list_node_type *node = search_adjacent_free();
	
	while (node != NULL)
	{
		compact_after(node);
		node = search_adjacent_free();
	}
}

list_node_type *get_unused_node()
{
	// this will return a node that can be recycled.
	// it'll check the unused nodes list to see if there's something there
	// if there's nothing then it'll search the free memory and used memory
	// lists to determine the highest virtual address in use, create a new node
	// and return that.
		
	list_node_type *result = NULL;
	
	// if there's a node available on the unused nodes list
	if (unused_nodes->first != NULL)
	{
		// make our result the first node on the unused nodes list
		result = unused_nodes->first;
		
		// remove that node from the list
		remove(unused_nodes, unused_nodes->first);
	}
	else
	{
		// i need to figure out the highest address available on the list
		u32int highest_virt_addr = highest_node_addr(free_memory);
		if (highest_node_addr(used_memory) > highest_virt_addr)
		{
			highest_virt_addr = highest_node_addr(used_memory);
		}
		
		u32int new_node_addr = highest_virt_addr + sizeof(list_type) + sizeof(list_node_type);
		
		u32int new_data_addr = new_node_addr + sizeof(list_node_type);
		
		result = (list_node_type *) new_node_addr;
		
		result->prev = NULL;
		result->data = (u32int *) new_data_addr;
		result->next = NULL;
		
		allocation_type *result_data = (allocation_type *) new_data_addr;
		
		result_data->virt_addr = NULL;
		result_data->size = NULL;
	}
	
	return result;
}

u32int highest_node_addr(list_type *list)
{
	// searches through a list and finds the highest used virt addr for a node
	list_node_type *candidate = list->first;
	u32int result = (u32int) candidate;
	
	do
	{
		if ((u32int) candidate > result)
		{
			result = (u32int) candidate;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

void print_all_free()
{
	if (free_memory->first != NULL)
	{
		print_allocation_list(free_memory);
	}
	else
	{
		put_str("\nEmpty list.");
	}
}

void print_all_used()
{
	if (used_memory->first != NULL)
	{
		print_allocation_list(used_memory);
	}
	else
	{
		put_str("\nEmpty list.");
	}
}

u32int *malloc(u32int size)
{
	// find a node i want to split
	list_node_type *malloc_node = search_free(size);
	
	// split it, and get the address for the node
	list_node_type *split_node = split_free(malloc_node, size);
	
	// remove it from the free list
	remove(free_memory, split_node);
	
	// put it on the used list
	insert_last(used_memory, split_node);
	
	// create a pointer to the new allocation
	allocation_type *malloc_node_data = (allocation_type *) malloc_node->data;
	u32int *malloc_ptr = (u32int *) malloc_node_data->virt_addr;
	
	// return the pointer
	return malloc_ptr;
}

list_node_type *search_free(u32int size)
{
	// searches for a free node that's greater than or equal to size
	list_node_type *result = NULL;
	list_node_type *candidate = free_memory->first;
	
	do
	{
		allocation_type *node_data = candidate->data;
		
		if (node_data->size >= size)
		{
			result = candidate;
			break;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

list_node_type *split_free(list_node_type *node, u32int size)
{
	// make a pointer to the node data
	allocation_type *node_data = node->data;
	
	// get a new node
	list_node_type *new_node = get_unused_node();
	
	// make a pointer to the new node data
	allocation_type *new_node_data = new_node->data;
	
	// figure out the virtual address for the new node
	new_node_data->virt_addr = node_data->virt_addr + size;
	
	// figure out the size of the new node
	new_node_data->size = node_data->size - size;
	
	// adjust the size of the existing node
	node_data->size = size;
	
	// insert the new node on the list after the existing node
	insert_after(free_memory, node, new_node);
	
	// return the node
	return node;
}

void free(u32int *virt_addr)
{
	// find the node on the used list for the virutal address we're freeing
	list_node_type *used_node = search_used((u32int) virt_addr);
	
	// find the node on the free list that it belongs ahead of
	list_node_type *goes_before = search_free_neighbor(used_node);
	
	// remove the used node from the used memory list
	remove(used_memory, used_node);
	
	// put it on the free memory list
	insert_before(free_memory, goes_before, used_node);
	
	// compact the list
	compact_all_free();
}

list_node_type *search_free_neighbor(list_node_type *node)
{
	// returns a result suitable for an insert before on the free memory list
	list_node_type *result = NULL;
	list_node_type *candidate = free_memory->first;
	
	do
	{
		// make a pointer to the candidate data
		allocation_type *candidate_data = candidate->data;
		
		// make a pointer to the node data
		allocation_type *node_data = node->data;
		
		if (candidate_data->virt_addr > node_data->virt_addr)
		{
			result = candidate;
			break;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

list_node_type *search_used(u32int virt_addr)
{
	// searched the used memory list for a node with a specific virtual
	// address associated with the region
	
	list_node_type *result = NULL;
	list_node_type *candidate = used_memory->first;
	
	do
	{
		allocation_type *candidate_data = candidate->data;
		
		if (candidate_data->virt_addr == virt_addr)
		{
			result = candidate;
			break;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}






































