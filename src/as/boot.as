# Declare constants used for the multiboot header
.set ALIGN,		1<<0
.set MEMINFO,	1<<1
.set MAGIC,		0x1BADB002
.set FLAGS,		ALIGN | MEMINFO
.set CHECKSUM,	-(MAGIC + FLAGS)

# Setting the virtual base address for the kernel
.set KERNEL_VIRTUAL_BASE,	0xC0000000					# 3G
.set KERNEL_PAGE_NUMBER,	(KERNEL_VIRTUAL_BASE >> 22)	# Page directory index for kernels page

.global .multiboot
.extern code
.extern bss
.extern end

.section .multiboot
	#.align 4
	.long MAGIC
	.long FLAGS
	.long CHECKSUM
	.long .multiboot
	.long code
	.long bss
	.long end
	.long _start

.section .data
	.align 0x1000
	BootPageDirectory:
		# this set up the page directory, and identity maps the first 4MB using 4KB pages
		.long 0x00000003
		.rept (KERNEL_PAGE_NUMBER - 1)
			.long 0
		.endr
		.long 0x00000003
		.rept (1024 - KERNEL_PAGE_NUMBER - 1)
			.long 0
		.endr
	
	
	
	
	
	
	
	
	

.section .text
	.global _start
	.extern kernel_main
	.type _start, @function
	_start:
		push %esp
		push %ebx
		cli
		call kernel_main
		.hang:
			jmp .hang

.size _start, . - _start













