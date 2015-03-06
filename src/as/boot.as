# Declare constants used for the multiboot header
.set ALIGN,		1<<0
.set MEMINFO,	1<<1
.set MAGIC,		0x1BADB002
.set FLAGS,		ALIGN | MEMINFO
.set CHECKSUM,	-(MAGIC + FLAGS)

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
