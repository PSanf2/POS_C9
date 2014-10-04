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

static u32int node_size = sizeof(node);

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
	// this is mainly for debugging. it's called by the kernel to print
	// the state of the free memory list on demand.
	print_forwards(free_mem);
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
	
	// i shouldn't have to get too fancy w/ the list.
	// i'll shrink the given node my adjusting it's size, saving the previous value
	// i'll create a pointer to the proper location for the new node
	// i'll populate the values for the new node
	// i'll insert the new node on the free memory list by inserting it after the given node
	// i'll return the pointer to the new node
	
	//print_node(myNode);
	
	node *newNode = (node *) myNode + node_size + bytes + 1;
	
	newNode->prev = NULL;
	newNode->data = (u32int *) newNode + node_size;
	newNode->size = (u32int *) myNode->size - bytes - node_size;
	newNode->next = NULL;
	
	myNode->size = (u32int *) bytes;
	
	insertAfter(free_mem, myNode, newNode);
	
	return newNode;
}

void compact_free()
{
	
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
		if ((u32int) candidate->size >= bytes)
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

void free(__attribute__((unused)) u32int *addr)
{
	
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
	node_ptr->data = (u32int *) (node_ptr + node_size);
	node_ptr->size = (u32int *) (available_mem - node_size);
	node_ptr->next = NULL;
	
	// put it on the free memory list
	insertBeginning(free_mem, node_ptr);
		
}
