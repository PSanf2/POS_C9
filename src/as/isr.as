.section .text

	# I NEED TO DEFINE ALL OF THE isr# FUNCTIONS
	# ILL BE USING A MACRO FOR THIS
	.macro ISR_NO_ERR_CODE arg1
		.global isr\arg1
		.type isr\arg1, @function
		isr\arg1:
			cli
			push $0
			push $\arg1
			jmp isr_common_stub
	.endm
	
	.macro ISR_ERR_CODE arg1
		.global isr\arg1
		.type isr\arg1, @function
		isr\arg1:
			cli
			push $\arg1
			jmp isr_common_stub
	.endm
	
	# putting the macro to use to define the isr functions
	ISR_NO_ERR_CODE 0
	ISR_NO_ERR_CODE 1
	ISR_NO_ERR_CODE 2
	ISR_NO_ERR_CODE 3
	ISR_NO_ERR_CODE 4
	ISR_NO_ERR_CODE 5
	ISR_NO_ERR_CODE 6
	ISR_NO_ERR_CODE 7
	ISR_ERR_CODE   8
	ISR_NO_ERR_CODE 9
	ISR_ERR_CODE   10
	ISR_ERR_CODE   11
	ISR_ERR_CODE   12
	ISR_ERR_CODE   13
	ISR_ERR_CODE   14
	ISR_NO_ERR_CODE 15
	ISR_NO_ERR_CODE 16
	ISR_NO_ERR_CODE 17
	ISR_NO_ERR_CODE 18
	ISR_NO_ERR_CODE 19
	ISR_NO_ERR_CODE 20
	ISR_NO_ERR_CODE 21
	ISR_NO_ERR_CODE 22
	ISR_NO_ERR_CODE 23
	ISR_NO_ERR_CODE 24
	ISR_NO_ERR_CODE 25
	ISR_NO_ERR_CODE 26
	ISR_NO_ERR_CODE 27
	ISR_NO_ERR_CODE 28
	ISR_NO_ERR_CODE 29
	ISR_NO_ERR_CODE 30
	ISR_NO_ERR_CODE 31


	# Tell the assembler theres an external symbol in use
	.extern isr_handler

	# isr common stub
	.global isr_common_stub
	.type isr_common_stub, @function
	isr_common_stub:
		pusha
		
		mov %ds, %ax
		push %eax
		
		mov $0x10, %ax
		mov %ax, %ds 
		mov %ax, %es
		mov %ax, %fs
		mov %ax, %gs
		
		call isr_handler
		
		pop %ebx
		mov %bx, %ds
		mov %bx, %es
		mov %bx, %fs
		mov %bx, %gs
		
		popa
		addl $8, %esp
		sti
		iret
