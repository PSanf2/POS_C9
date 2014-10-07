#include <memory.h>
#include <vga.h>

/*
 * This is my implementation of a first-fit memory manager.
 * All of the functions defined in this file assume that require variables
 * have been initialized properly. It is the kernel programmer's responsibility
 * to avoid doing stupid things.
 */

// used for determining where the kernel ends
extern u32int end;
static u32int kernel_end = (u32int) &end;

static u32int mem_upper_amount;	// this is the amount of high memory in kilobytes starting at the 1M mark.
static u32int first_mem_addr;	// the first address after the kernel
static u32int last_mem_addr;	// this should be the address of the last byte of high memory available.
static u32int available_mem;	// this is the amount of memory available to the system.

static list *free_mem;

void memcpy(u8int *dest, const u8int *src, u32int len)
{
	const u8int *sp = (const u8int *) src;
	u8int *dp = (u8int *) dest;
	for (; len != 0; len--)
		*dp++ = *sp++;
}

void memset(u8int *dest, u8int val, u32int len)
{
	u8int *temp = (u8int *) dest;
	for (; len != 0; len--)
		*temp++ = val;
}

void print_node(node *myNode)
{
	vga_buffer_put_str("\nNode at address ");
	vga_buffer_put_dec((u32int) myNode);
	vga_buffer_put_str("\nPrev: ");
	vga_buffer_put_dec((u32int) myNode->prev);
	vga_buffer_put_str("\nData: ");
	vga_buffer_put_dec((u32int) myNode->data);
	vga_buffer_put_str("\nSize: ");
	vga_buffer_put_dec((u32int) myNode->size);
	vga_buffer_put_str("\nNext: ");
	vga_buffer_put_dec((u32int) myNode->next);
	vga_buffer_put_str("\n");
}

void print_forwards(list *myList)
{
	node *myNode = myList->first;
	while (myNode != NULL)
	{
		print_node(myNode);
		myNode = myNode->next;
	}
}

void print_mem_state()
{
	print_forwards(free_mem);
}

void print_node_state(u32int *myNodeAddr)
{
	print_node((node *) myNodeAddr);
}

void insertAfter(list *myList, node *myNode, node *newNode)
{
	newNode->prev = myNode;
	newNode->next = myNode->next;
	if (myNode->next == NULL)
	{
		myList->last = newNode;
	}
	else
	{
		myNode->next->prev = newNode;
	}
	myNode->next = newNode;
}

void insertBefore(list *myList, node *myNode, node *newNode)
{
	newNode->prev = myNode->prev;
	newNode->next = myNode;
	if (myNode->prev == NULL)
	{
		myList->first = newNode;
	}
	else
	{
		myNode->prev->next = newNode;
	}
	myNode->prev = newNode;
}

void insertBeginning(list *myList, node *newNode)
{
	if (myList->first == NULL)
	{
		myList->first = newNode;
		myList->last = newNode;
		newNode->prev = NULL;
		newNode->next = NULL;
	}
	else
	{
		insertBefore(myList, myList->first, newNode);
	}
}

void insertEnd(list *myList, node *newNode)
{
	if (myList->last == NULL)
	{
		insertBeginning(myList, newNode);
	}
	else
	{
		insertAfter(myList, myList->last, newNode);
	}
}

void remove(list *myList, node *myNode)
{
	if (myNode->prev == NULL)
	{
		myList->first = myNode->next;
	}
	else
	{
		myNode->prev->next = myNode->next;
	}
	
	if (myNode->next == NULL)
	{
		myList->last = myNode->prev;
	}
	else
	{
		myNode->next->prev = myNode->prev;
	}
}

node *split_free(node *myNode, u32int bytes)
{
	/*
	 * this will accept a free node as a parameter, and a u32int number.
	 * this will split the given node at bytes, and return the address.
	 * to the new node.
	 * 
	 * BEFORE
	 *   ---------------------------------------------------------------
	 *   |       |                                                     |
	 *   ---------------------------------------------------------------
	 *   A       B                                                     C
	 * A is where the node data starts.
	 * B is where the allocated memory starts.
	 * C is where the allocated memory ends.
	 * 
	 * AFTER
	 *   ---------------------------------------------------------------
	 *   |       |                       |       |                     |
	 *   ---------------------------------------------------------------
	 *   A       B                       C       D                     E
	 * A and B are the same as above.
	 * C is
	 * 	The end of the original free block.
	 * 	Where the node data for the new block now starts
	 *  Where the split occured as calculated by C = B + bytes
	 * D is where the allocated memory for the new portion starts.
	 * E is where the allocated memory ends.
	 * 
	 * C will be returned.
	 * 
	 * This function will be responsible for properly updating the free_mem list with the changes.
	 */
	
	u32int newNodeAddr;
	
	// the math used in this if/else was arrived at through trial-and-error. 
	if (myNode == free_mem->first)
	{
		newNodeAddr = (((u32int) myNode) + sizeof(node) + bytes + 1 - 17) + 256;
	}
	else
	{
		newNodeAddr = (((u32int) myNode) + sizeof(node) + bytes + 1 - 17) + 64;
	}
	
	node *newNode = (node *) newNodeAddr;
	
	newNode->prev = NULL;
	newNode->data = (u32int *) newNode + sizeof(node);
	
	u32int newSize = (u32int) myNode->size;
	newSize = newSize - bytes;
	newSize = newSize - sizeof(node);
	
	if (newSize >= 48)
	{
		newSize = newSize - 48;
	}
	
	newNode->size = (u32int *) newSize;
	
	newNode->next = NULL;
	myNode->size = (u32int *) bytes;
	
	insertAfter(free_mem, myNode, newNode);
	
	// there's some kind of compounding error in the split operation. after the initial node is split, every subsequent split of the last node causes
	// it to be 192 (3 * 64) bytes too large. To fix this we just make sure that the data + the size of the last node = the last memory address.
	// if data + size > last mem addr then size = last mem addr - data
	node *lastNode = free_mem->last;
	u32int lastData = (u32int) lastNode->data;
	u32int lastSize = (u32int) lastNode->size;
	if ((lastData + lastSize) > last_mem_addr)
	{
		lastNode->size = (u32int *) (last_mem_addr - lastData);
	}
	
	return newNode;
}

void compact_free(node *myNode)
{
	// i need to go over the list looking at each node's data address and size.
	// if the node's data address + size = the next node then the free node in question, and the one following it, are adjacent.
	// if two free nodes are adjacent then i need to compact them together.
	
	/*u32int newSize = (u32int) myNode->size;
	newSize = newSize - bytes;
	newSize = newSize - sizeof(node);
	 */
	 
	 node *nextNode = myNode->next;
	 
	 u32int newSize = (u32int) myNode->size;
	 u32int nextSize = (u32int) nextNode->size;
	 newSize = newSize + nextSize + 64;
	 
	 myNode->size = (u32int *) newSize;
	 
	 remove(free_mem, nextNode);
}

node *search_adjacent_free_node()
{
	// this function will for the first node that can be compacted with the node that imemdiately follows it.
	
	node *result = NULL;
	
	node *candidate = free_mem->first;
	
	do
	{
		if (((u32int) candidate->data + (u32int) candidate->size) == (u32int) candidate->next)
		{
			result = candidate; 
			break;
		}
		
		candidate = candidate->next;
	} while (candidate != NULL);
	
	return result;
	
}

void compact_all_free()
{
	// this will repeatedly go over the list, compacting all adjacent free nodes
	
	node *myNode = search_adjacent_free_node();
	
	while ((u32int) myNode != NULL)
	{
		compact_free(myNode);
		myNode = search_adjacent_free_node();
	}
	
}

void compact_free_mem()
{
	compact_all_free();
}

u32int *malloc(u32int bytes)
{
	/*
	 * This assumes that the free memory list has been properly initialized.
	 * 
	 * I need to go over the list until I find a node that's larger than bytes.
	 * 	If there isn't a free node large enough then I'll return NULL.
	 * I need to split that node.
	 * I need to remove the node from the list.
	 * I need to return a pointer to the node.
	 */
	
	u32int result = NULL;
	
	node *candidate = free_mem->first;
	
	do
	{
		if ((u32int) candidate->size == bytes)
		{
			remove(free_mem, candidate);
			candidate->prev = NULL;
			candidate->next = NULL;
			result = (u32int) candidate->data;
			break;
		}
		// we need to make sure that the candidate node is at least 48 bytes larger than the requested size or else there will be memory leaks and corruption.
		// the 48 is due to the pointer arithmetic that's needed to keep things straight on the list.
		// i think the issue ultimately has to do with how sizeof() interprets the size of the node data type.
			// most of the time it comes back as 16, but is blatantly wrong in some instances.
			// (the node data type containts two u32int pointers, and two pointers to other nodes making it 128 bytes on the first node, and 64 on all others).
			// i think this has to do with how a node was most recently initialized before the sizeof() function is called.
			// to overcome this I need to use constant values at select places to adjust the values of47 pointers to bump them back to the right place.
			// the ultimate result is that if a node needs to be split to allocate some memory, and the resulting node is less than 48 bytes, then the size
			// of the new node will not be calculated correctly, cause bad things to happen, result in memory leaks whenever new memory is allocated pr freed, and
			// ultimately corrupt the memory. You'll end up with nodes that either have no size information, or think the node address is also the size of the
			// node (and making it appear that the node is much larger than it actually is).
		// you can allocate memory as small as you want, but things get a little iffy for allocation requests of less than 128 bytes.
		// things work for the most part, but some node occationally get a 0 size, and leak memory.
		// if the node of free memory doesn't meet the minimum size requirements for the allocation then it's not a "fit," and going to the next
		// node is still within the definition of "first fit."
		else if ((u32int) candidate->size > (bytes + 48))
		{
			split_free(candidate, bytes);
			remove(free_mem, candidate);
			candidate->prev = NULL;
			candidate->next = NULL;
			result = (u32int) candidate->data;
			break;
		}
		else
		{
			candidate = candidate->next;
		}
	} while (candidate != NULL);
	
	return (u32int *) result;
}

void free(u32int *addr)
{
	node *myNode;
	if ((u32int) addr == (first_mem_addr + 256))
	{
		myNode = (node *) ((u32int) addr - 256);
	}
	else
	{
		myNode = (node *) ((u32int) addr - 64);
	}
	
	node *candidate = free_mem->first;
	
	do
	{
		if ((u32int) candidate > (u32int) myNode)
		{
			insertBefore(free_mem, candidate, myNode);
			break;
		}
		candidate = candidate->next;
	} while (candidate != NULL);
	
}

void memory_manager_initialize(struct multiboot *mboot_ptr)
{
	mem_upper_amount = mboot_ptr->mem_upper;			// this is the amount of high memory in kilobytes starting at the 1M mark.
	first_mem_addr = kernel_end + 1;					// the first address after the kernel.
	last_mem_addr = (mem_upper_amount * 1024) - 1;		// this should be the address of the last byte of high memory available.
	available_mem = last_mem_addr - first_mem_addr;		// this is the amount of memory, in byres, available to the system.
	
	// block of print statements for debugging.
	vga_buffer_put_str("Initializing memory management...");
	vga_buffer_put_str("\nKernel ends at ");
	vga_buffer_put_dec(kernel_end);
	vga_buffer_put_str("\nThere is ");
	vga_buffer_put_dec(mem_upper_amount);
	vga_buffer_put_str("K of memory starting from the 1M mark.");
	vga_buffer_put_str("\nFirst free address: ");
	vga_buffer_put_dec(first_mem_addr);
	vga_buffer_put_str("\nLast free address: ");
	vga_buffer_put_dec(last_mem_addr);
	vga_buffer_put_str("\nThere is ");
	vga_buffer_put_dec(available_mem);
	vga_buffer_put_str(" bytes of memory available to the system.");
	vga_buffer_put_str("\n");
	
	// the memory will be managed by putting the information needed for a node before the memory that will be allocated.
	// i need to put a node at the first free address.
	// the memory allocated will actually be the entire memory minus the size of the node data type.
		// (it's 4 u32int * values, so it should be 32 bits * 4 = 128 bits, or 16 bytes)
	/*
	 *   ---------------------------------------------------------------
	 *   |       |                                                     |
	 *   ---------------------------------------------------------------
	 *   A       B                                                     C
	 */
	// The space between A and B is the 16 bytes needed for a node
	// The space between A and C is the allocated memory
	// A is data - sizeof(node)
	// B is data
	// C is data + size
	
	// clear the list to make sure there are no funky pointers
	free_mem->first = NULL;
	free_mem->last = NULL;
	
	// make a pointer to the first address in memory
	node *node_ptr = (node *) first_mem_addr;
	
	// set the values
	node_ptr->prev = NULL;
	node_ptr->data = (u32int *) (node_ptr + sizeof(node));
	node_ptr->size = (u32int *) (available_mem - sizeof(node) - 256 + 16);
	node_ptr->next = NULL;
	
	// put it on the free memory list
	insertBeginning(free_mem, node_ptr);
	
}
