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
		
		# is this line needed? dunno. getting it from the osdev wiki. yuri doesn't have it in his code.
		mov %ebp, %esp
		
		pop %ebp
		ret

	.global read_cr2
	.type read_cr2, @function
	read_cr2:
		mov %cr2, %eax
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
		
		# is this line needed? dunno. getting it from the osdev wiki. yuri doesn't have it in his code.
		mov %ebp, %esp
		
		pop %ebp
		ret
