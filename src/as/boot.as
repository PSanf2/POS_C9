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
	# Set up the boot page directory for 4KB pages
	.align 0x1000
	BootPageDirectory:
		# The first page directory entry, attributes read/write, present
		.long 0x00000083
		# Empty PDEs
		.fill (KERNEL_PAGE_NUM - 1), 4, 0x00000000
		# The PDE for the kernel's virtual address, attributes read/write, present
		.long 0x00000083
		# Empty PDEs
		.fill (1024 - KERNEL_PAGE_NUM - 1), 4, 0x00000000

.section .setup
	.global _loader
	_loader:
		# Put the address of the page directory on CR3
		lea (BootPageDirectory), %ecx
		sub $KERNEL_VIRTUAL_BASE, %ecx
		movl %ecx, %cr3
		
		# Set the PSE bit on CR4 to enable 4MB pages (I can change this in the kernel to use 4KB pages)
		movl %cr4, %ecx
		orl $0x00000010, %ecx
		movl %ecx, %cr4
		
		# Set the paging enable bit on CR0
		movl %cr0, %ecx
		orl $0x80000000, %ecx
		movl %ecx, %cr0
		
		lea (_startHigherHalf), %ecx
		jmp *%ecx

.section .text
	.extern kernel_main
	_startHigherHalf:
		# unmapped the first 4MB of virtual addresses, which were identity mapped
		movl $0x00000000, (BootPageDirectory)
		invlpg (0)
		
		movl $stackTop, %esp
		
		push %esp
		push %ebx
		cli
		call kernel_main
		
		.hang:
			jmp .hang
