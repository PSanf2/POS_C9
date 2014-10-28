.section .text
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

	.global read_cr1
	.type read_cr1, @function
	read_cr1:
		mov %cr1, %eax
		ret

	.global write_cr1
	.type write_cr1, @function
	write_cr1:
		push %ebp
		mov %esp, %ebp
		mov 8(%ebp), %eax
		mov %eax, %cr1
		pop %ebp
		ret

	.global read_cr2
	.type read_cr2, @function
	read_cr2:
		mov %cr2, %eax
		ret

	.global write_cr2
	.type write_cr2, @function
	write_cr2:
		push %ebp
		mov %esp, %ebp
		mov 8(%ebp), %eax
		mov %eax, %cr2
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

	.global read_cr4
	.type read_cr4, @function
	read_cr4:
		mov %cr4, %eax
		ret

	.global write_cr4
	.type write_cr4, @function
	write_cr4:
		push %ebp
		mov %esp, %ebp
		mov 8(%ebp), %eax
		mov %eax, %cr4
		pop %ebp
		ret
