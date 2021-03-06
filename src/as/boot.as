# Declare constants used for the multiboot header
.set ALIGN,		1<<0
.set MEMINFO,	1<<1
.set MAGIC,		0x1BADB002
.set FLAGS,		ALIGN | MEMINFO
.set CHECKSUM,	-(MAGIC + FLAGS)

# Stuff I need for the multiboot header.
.section .multiboot
	.align 4
	.long MAGIC
	.long FLAGS
	.long CHECKSUM
	.align 0x1000

# stuff for a stack
.section .stack, "aw", @nobits
	stackBottom:
		.skip 0x4000
	stackTop:

# Constants needed to translate physical addresses to virtual addresses
.set KERNEL_VIRTUAL_BASE,	0xC0000000
.set KERNEL_PAGE_NUM,		(KERNEL_VIRTUAL_BASE >> 22);

.section .data
	# Set up the boot page directory for 4MB pages
	.align 0x1000
	.global BootPageDirectory
	BootPageDirectory:
		# The first page directory entry, attributes read/write, present
		.long 0x00000083
		# Empty PDEs
		.fill (KERNEL_PAGE_NUM - 1), 4, 0x00000000
		# The PDE for the kernel's virtual address, attributes read/write, present
		.long 0x00000083
		# Empty PDEs
		.fill (1024 - KERNEL_PAGE_NUM - 1), 4, 0x00000000

# Declare the setup function that GRUB will call first.
.section .setup, "ax", @progbits
	.global _loader
	_loader:
		
		# Put the address of the page directory on CR3
		lea (BootPageDirectory - KERNEL_VIRTUAL_BASE), %ecx
		movl %ecx, %cr3
		
		# Set the PSE bit on CR4 to enable 4MB pages (I can change this in the kernel to use 4KB pages)
		movl %cr4, %ecx
		orl $0x00000010, %ecx
		movl %ecx, %cr4
		
		# Set the paging enable bit on CR0
		movl %cr0, %ecx
		orl $0x80000000, %ecx
		movl %ecx, %cr0
		
		# Get the effective address of _startHigherHalf, which is located at the high virtual address 0xC010000
		lea (_startHigherHalf), %ecx
		# Jump to the next function
		jmp *%ecx

# Declare the function that will be used to start the kernel.
.section .text
	.extern kernel_main
	_startHigherHalf:
		
		# Unmap the first 4MB of virtual addresses, which were identity mapped.
		movl $0x00000000, (BootPageDirectory)
		invlpg (0)
		
		# Set up the parameters that will be passed to the kernel.
		# Push the address of the stack on to the stack.
		movl $stackTop, %esp
		push %esp
		
		# Push the address of the multiboot header onto the stack.
		add $KERNEL_VIRTUAL_BASE, %ebx
		push %ebx
		
		# Disable the interupts.
		cli
		
		# Call the kernel.
		call kernel_main
		
		# Hang up if something goes wrong.
		.hang:
			jmp .hang
