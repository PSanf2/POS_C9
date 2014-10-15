.section .text
	.global update_segregs
	.type update_segregs, @function
	update_segregs:
		push %eax
		mov $0x10, %ax
		mov %ax, %ds 
		mov %ax, %es
		mov %ax, %fs
		mov %ax, %gs
		ljmp $0x08, $.segregs_complete_flush
		.segregs_complete_flush:
		pop %eax
		ret

	.global read_cr0
	.type read_cr0, @function
	read_cr0:
		mov %cr0, %eax
		ret

	.global write_cr0
	.type write_cr0, @function
	write_cr0:
		push %ebp
		mov %esp, %ebp
		mov 8(%ebp), %eax
		mov %eax, %cr0
		pop %ebp
		ret

	.global read_cr3
	.type read_cr3, @function
	read_cr3:
		mov %cr3, %eax
		ret

	.global write_cr3
	.type write_cr3, @function
	write_cr3:
		push %ebp
		mov %esp, %ebp
		mov 8(%ebp), %eax
		mov %eax, %cr3
		pop %ebp
		ret
