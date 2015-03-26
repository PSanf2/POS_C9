#include <vmm.h>

static vmm_list *free_mem = (vmm_list *) 0xFF800000;

void vmm_initialize()
{
	// PD[1022] and PD[1023] will be off limits, and always "allocated."
	
	free_mem->first = NULL;
	free_mem->last = NULL;
	
	// rename the pointer to my current page directory to make it easier to work with.
	u32int *page_directory = current_page_directory->virt_addr;
	
	// figure out the address where the first node will go.
	
	// for each entry on the page directory
	// this loop only runs to less than 1022 because PD[1023] is reserved for the recursive mappings,
	// and PD[1022] is reserved for the free memory list, and bitmap.
	for (u32int i = 0; i < 1022; i++)
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
			insert_end(new_node);
			
			// compact the list
			compact_all();
			
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
					insert_end(new_node);
					
					// compact the list
					compact_all();
					
				}
			}
		}
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
	
	put_str("\n\tvirt_addr=");
	put_hex(node->virt_addr);
	put_str(" size=");
	put_hex(node->size);
}

void insert_after(vmm_node *node, vmm_node *new_node)
{
	new_node->prev = node;
	new_node->next = node->next;
	if (node->next == NULL)
	{
		free_mem->last = new_node;
	}
	else
	{
		node->next->prev = new_node;
	}
	node->next = new_node;
}

void insert_before(vmm_node *node, vmm_node *new_node)
{
	new_node->prev = node->prev;
	new_node->next = node;
	if (node->prev == NULL)
	{
		free_mem->first = new_node;
	}
	else
	{
		node->prev->next = new_node;
	}
	node->prev = new_node;
}

void insert_beginning(vmm_node *node)
{
	if (free_mem->first == NULL)
	{
		free_mem->first = node;
		free_mem->last = node;
		node->prev = NULL;
		node->next = NULL;
	}
	else
	{
		insert_before(free_mem->first, node);
	}
}

void insert_end(vmm_node *node)
{
	if (free_mem->last == NULL)
	{
		insert_beginning(node);
	}
	else
	{
		insert_after(free_mem->last, node);
	}
}

void remove(vmm_node *node)
{
	if (node->prev == NULL)
	{
		free_mem->first = node->next;
	}
	else
	{
		node->prev->next = node->next;
	}
	
	if (node->next == NULL)
	{
		free_mem->last = node->prev;
	}
	else
	{
		node->next->prev = node->prev;
	}
}

vmm_node *split(__attribute__((unused)) vmm_node *node, __attribute__((unused)) u32int bytes)
{
	return 0;
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
	
	remove(next_node);
}

void compact_all()
{
	vmm_node *node = search_adjacent_free();
	
	while (node != NULL)
	{
		compact_after(node);
		node = search_adjacent_free();
	}
}








