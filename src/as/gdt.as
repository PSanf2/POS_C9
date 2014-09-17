.section .text
	.global gdt_flush
	.type gdt_flush, @function
	gdt_flush:
		mov 4(%esp), %eax
		lgdt (%eax)
		movw $0x10, %ax
		movw %ax, %ds
		movw %ax, %es
		movw %ax, %fs
		movw %ax, %gs
		movw %ax, %ss
		ljmp $0x08, $.gdt_complete_flush
		.gdt_complete_flush:
		ret
