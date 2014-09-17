.section .text

	.macro IRQ arg1 arg2
		.global irq\arg1
		.type irq\arg1, @function
		irq\arg1:
			cli
			push $0
			push $\arg2
			jmp irq_common_stub
	.endm
	
	IRQ   0,    32
	IRQ   1,    33
	IRQ   2,    34
	IRQ   3,    35
	IRQ   4,    36
	IRQ   5,    37
	IRQ   6,    38
	IRQ   7,    39
	IRQ   8,    40
	IRQ   9,    41
	IRQ  10,    42
	IRQ  11,    43
	IRQ  12,    44
	IRQ  13,    45
	IRQ  14,    46
	IRQ  15,    47
	
	.extern irq_handler
	
	.global irq_common_stub
	.type irq_common_stub, @function
	irq_common_stub:
		pusha
		
		mov %ds, %ax
		push %eax
		
		mov $0x10, %ax
		mov %ax, %ds 
		mov %ax, %es
		mov %ax, %fs
		mov %ax, %gs
		
		call irq_handler
		
		pop %ebx
		mov %bx, %ds
		mov %bx, %es
		mov %bx, %fs
		mov %bx, %gs
		
		popa
		addl $8, %esp
		sti
		iret
