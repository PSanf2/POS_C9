#include <vmm.h>

// i need to have a free and used memory list
// the used memory list should never be compacted.
// it'll be needed to store information about which memory ranges have been allocated.
// when memory is allocated a free node will be split, the node for the allocated memory
// will be removed from the free list, and then put on the used list.
// freeing memory will be a reverse process where the node is put on the free list, and the list
// will be compacted.

// don't fall in to that trap with the search function causing a page fault trying to read from a null pointer

static vmm_list *used_mem = (vmm_list *) 0xFF400000; // where the used list will start
static vmm_list *free_mem = (vmm_list *) 0xFF400000 + sizeof(vmm_list); // where the free list will start

void vmm_initialize()
{
	// these regions are off limits for allocations
	// PD[1021] = mappings for the lists
	// PD[1022] = mappings for the bitmap
	// PD[1023] = mappings for the page tables
	
	put_str("\nVirtual memory manager initialized.");
	
	used_mem->first = NULL;
	used_mem->last = NULL;
	
	free_mem->first = NULL;
	free_mem->last = NULL;
	
	// rename the pointer to my current page directory to make it easier to work with.
	u32int *page_directory = current_page_directory->virt_addr;
	
	// figure out the address where the first node will go.
	
	// for each entry on the page directory
	// this loop only runs to less than 1022 because PD[1023] is reserved for the recursive mappings,
	// and PD[1022] is reserved for the free memory list, and bitmap.
	for (u32int i = 0; i < 1021; i++)
	{
		
		// if there's nothing on that index of the page directory
		if ((page_directory[i] & 0x1) == 0)
		{
			
			// figure out the address of the node
			u32int free_addr = i * 0x400000;
			// figure out the size of the node to add to the list
			u32int free_size = 0x400000;
			
			// figure out the virtual address where the new node will live
			u32int new_node_addr;
			if (free_mem->last == NULL)
			{
				new_node_addr = (u32int) free_mem + sizeof(vmm_list);
			}
			else
			{
				new_node_addr = (u32int) free_mem->last + sizeof(vmm_node);
			}
			
			// make a pointer to the place where the new node will live
			vmm_node *new_node = (vmm_node *) new_node_addr;
			
			// populate the important values on the node
			new_node->virt_addr = free_addr;
			new_node->size = free_size;
			
			// add the node to the end of the list
			insert_last(free_mem, new_node);
			
			// compact the list
			compact_all_free();
			
		}
		else
		{
			// there's an value for that page directory entry, so i need to go over the page table i found.
			
			// figure out the recursive address for the page table
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
					// figure out the address of the node
					u32int free_addr = ((i * 0x400000) + (j * 0x1000));
					// figure out the size of the node to add to the list
					u32int free_size = 0x1000;
					
					// figure out the virtual address where the new node will live
					u32int new_node_addr;
					if (free_mem->last == NULL)
					{
						new_node_addr = (u32int) free_mem + sizeof(vmm_list);
					}
					else
					{
						new_node_addr = (u32int) free_mem->last + sizeof(vmm_node);
					}
					
					// make a pointer to the place where the new node will live
					vmm_node *new_node = (vmm_node *) new_node_addr;
					
					// populate the important values on the node
					new_node->virt_addr = free_addr;
					new_node->size = free_size;
					
					// add the node to the end of the list
					insert_last(free_mem, new_node);
					
					// compact the list
					compact_all_free();
					
				}
			}
		}
	}
	
	put_str("\nVMM initialization done.\n");
}

u32int *malloc(u32int size)
{
	// find a node i want to split
	vmm_node *malloc_node = search_free(size);
	
	// split it, and get the address for the new node
	vmm_node *split_node = split_free(malloc_node, size);
	
	// remove it from the free list
	remove(free_mem, split_node);
	
	// put it on the used list
	insert_last(used_mem, split_node);
	
	// create a pointer to the new allocation
	u32int *malloc_ptr = (u32int *) split_node->virt_addr;
	
	// return the pointer
	return malloc_ptr;
}

vmm_node *search_free(u32int size)
{
	// searches for a free node that's >= size
	vmm_node *result = NULL;
	vmm_node *candidate = free_mem->first;
	
	do
	{
		if (candidate->size >= size)
		{
			result = candidate;
			break;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

u32int highest_addr(vmm_list *list)
{
	// searches through a list, and finds the highest used virt addr of any node.
	vmm_node *candidate = list->first;
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

vmm_node *split_free(vmm_node *node, u32int size)
{
	
	// figure out the addrss for the new node on the free list
	u32int new_node_addr = highest_addr(free_mem) + sizeof(vmm_node);
	
	// create a pointer to the new node
	vmm_node *new_node = (vmm_node *) new_node_addr;
	
	// figure out the virtual address for the new node
	new_node->virt_addr = node->virt_addr + size;
	
	// figure out the size of the new node
	new_node->size = node->size - size;
	
	// adjust the size of the existing node
	node->size = size;
	
	// insert the new node on the list after the existing node
	insert_after(free_mem, node, new_node);
	
	// return the new node
	return node;
	
}

void free(u32int *virt_addr)
{
	// i have to find the node for the virtual address on the used memory list
	// i have to figure out where it's supposed to go on the free memory list
	// i have to remove it from the used memory list
	// i have to put it on the free memory list in the proper place
	// i have to compact the free memory list.
	
	// find the node on the used list that for the virtual address we're freeing.
	vmm_node *used_node = search_used((u32int) virt_addr);
	
	// find the node on the free list that the used node belongs ahead of
	vmm_node *goes_before = search_free_neighbor(used_node);
	
	// remove the used node from the used memory list
	remove(used_mem, used_node);
	
	// put it on the free memory list
	insert_before(free_mem, goes_before, used_node);
	
	// compact the list
	compact_all_free();
	
}

vmm_node *search_free_neighbor(vmm_node *node)
{
	// returns the result suitable for an insert before on result.
	vmm_node *result = NULL;
	vmm_node *candidate = free_mem->first;
	
	do
	{
		if (candidate->virt_addr > node->virt_addr)
		{
			result = candidate;
			break;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

vmm_node *search_used(u32int virt_addr)
{
	// searches the used list for a node with a specific virtual address associated with a region
	vmm_node *result = NULL;
	vmm_node *candidate = used_mem->first;
	
	do
	{
		if (candidate->virt_addr == virt_addr)
		{
			result = candidate;
			break;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

void insert_after(vmm_list *list, vmm_node *node, vmm_node *new_node)
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

void insert_before(vmm_list *list, vmm_node *node, vmm_node *new_node)
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

void insert_first(vmm_list *list, vmm_node *new_node)
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

void insert_last(vmm_list *list, vmm_node *new_node)
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

void remove(vmm_list *list, vmm_node *node)
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

vmm_node *search_adjacent_free()
{
	vmm_node *result = NULL;
	vmm_node *candidate = free_mem->first;
	
	do
	{
		if ((candidate->next != NULL) && ((candidate->virt_addr + candidate->size) == (candidate->next->virt_addr)))
		{
			result = candidate;
			break;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
}

void compact_after(vmm_node *node)
{
	// compacts a node with the one after it
	vmm_node *next_node = node->next;
	
	u32int new_size = node->size + next_node->size;
	
	node->size = new_size;
	
	remove(free_mem, next_node);
}

void compact_all_free()
{
	vmm_node *node = search_adjacent_free();
	
	while (node != NULL)
	{
		compact_after(node);
		node = search_adjacent_free();
	}
}

void print_node(vmm_node *node)
{
	put_str("\nprev=");
	put_hex((u32int) node->prev);
	
	put_str(" node=");
	put_hex((u32int) node);
	
	put_str(" next=");
	put_hex((u32int) node->next);
	
	put_str(" virt_addr=");
	put_hex(node->virt_addr);
	
	put_str(" size=");
	put_hex(node->size);
}

void print_all(vmm_list *list)
{
	vmm_node *current = list->first;
	do
	{
		print_node(current);
		current = current->next;
	} while (current != NULL);
}

void print_all_free()
{
	put_str("\n\tfree_mem=");
	put_hex((u32int) free_mem);
	put_str(" first=");
	put_hex((u32int) free_mem->first);
	put_str(" last=");
	put_hex((u32int) free_mem->last);
	
	if (free_mem->first != NULL)
	{
		print_all(free_mem);
	}
	else
	{
		put_str("\nEmpty list.");
	}
}

void print_all_used()
{
	put_str("\n\tused_mem=");
	put_hex((u32int) used_mem);
	put_str(" first=");
	put_hex((u32int) used_mem->first);
	put_str(" last=");
	put_hex((u32int) used_mem->last);
	
	if (used_mem->first != NULL)
	{
		print_all(used_mem);
	}
	else
	{
		put_str("\nEmpty list.");
	}
}




