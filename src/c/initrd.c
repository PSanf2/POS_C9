#include <initrd.h>

u32int *tar_headers;
u32int header_count;

void initrd_initialize(struct multiboot *mboot_ptr)
{
	put_str("\nNumber of modules loaded: ");
	put_dec(mboot_ptr->mods_count);
	
	if (mboot_ptr->mods_count < 1)
	{
		put_str("\nGRUB did not load any modules! Unable to initialize initial RAM disk!");
		put_str("\nHalting.");
		for (;;) {}
	}
	
	put_str("\nmboot_ptr->mods_addr: ");
	put_hex(mboot_ptr->mods_addr);
	
	u32int initrd_addr = mboot_ptr->mods_addr + 0xC0000000;
	put_str("\ninitrd_addr: ");
	put_hex(initrd_addr);
	
	u32int *initrd_ptr = (u32int *) initrd_addr;
	put_str("\ninitrd physical address: ");
	put_hex(*initrd_ptr);
	
	*initrd_ptr = *initrd_ptr + 0xC0000000;
	put_str("\ninitrd_ptr: ");
	put_hex(*initrd_ptr);
	
	tar_header_type *first_header = (tar_header_type *) *initrd_ptr;
	
	header_count = count_headers((u32int) first_header);
	
	put_str("\nHeader count: ");
	put_dec(header_count);
	
	put_str("\nsizeof(u32int): ");
	put_hex(sizeof(u32int));
	
	tar_headers = malloc(header_count * sizeof(u32int));
	
	put_str("\nAddress allocated for headers: ");
	put_hex((u32int) tar_headers);
	
	vmm_print_used();
	
	// i need to parse the headers, and put a pointer to each one of them on the tar_headers "array."
	// access tar_headers as if it were an array, and put u32int pointers on it.
	
	put_str("\n");
}

void print_tar_header(tar_header_type *tar_header)
{
	put_str("\n\tTAR HEADER");
	
	put_str("\nHeader address: ");
	put_hex((u32int) tar_header);
	
	put_str("\nFilename: ");
	put_str(tar_header->name);
	
	put_str("\nRaw octal size: ");
	put_str(tar_header->size);
	
	put_str("\nCalculated decimal size: ");
	put_dec(get_size(tar_header->size));
	
}

void print_file_contents(tar_header_type *tar_header)
{
	put_str("\n\tFILE CONTENTS\n");
	
	u32int file_addr = (u32int) tar_header->name;
	file_addr = file_addr + 512;
	
	u32int *file_ptr = (u32int *) file_addr;
	
	put_str((char *) file_ptr);
}

u32int get_size(const char *in)
{
	u32int size = 0;
	u32int count = 1;
	
	for (u32int j = 11; j > 0; j--, count *= 8)
	{
		size += ((in[j - 1] - '0') * count);
	}
	
	return size;
}

u32int count_headers(u32int header_addr)
{
	tar_header_type *header = (tar_header_type *) header_addr;
	u32int count = 0;
	
	do
	{
		count++;
		
		u32int size = get_size(header->size);
		header_addr += ((size / 512) + 1) * 512;
		
		if (size % 512)
		{
			header_addr += 512;
		}
		
		header = (tar_header_type *) header_addr;
	} while(header->name[0] != '\0');
	
	return count;
}










