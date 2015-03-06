ENTRY(_start)

SECTIONS
{
	
	. = 0x00100000;
	
	.setup BLOCK(0x1000) : ALIGN(0x1000)
	{
		*(.multiboot)
		*(.setup)
	}
	
	. += 0xC0000000;
	
	start = .;
	
	.text : AT(ADDR(.text) - 0xC0000000)
	{
		*(.text)
	}
	
	.rodata ALIGN(0x1000) : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata)
	}
	
	.data ALIGN(0x1000) : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}
	
	.bss ALIGN(0x1000) : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.bss)
		*(.stack)
	}
	
	end = .;
}
