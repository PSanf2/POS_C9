ENTRY(_loader)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
	
	. = 0x00100000;
	
	.setup : ALIGN(0x1000)
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
	
	.rodata : AT(ADDR(.rodata) - 0xC0000000)
	{
		*(.rodata)
	}
	
	.data : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
	}
	
	.bss : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.bss)
		*(.stack)
	}
	
	end = .;
}
