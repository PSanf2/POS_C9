ENTRY(_start)

SECTIONS
{
	. = 0xC0100000;
	start = .;
	.text : AT(ADDR(.text) - 0xC0000000)
	{
		*(.multiboot)
		code = .;
		*(.text)
	}

	.data : AT(ADDR(.data) - 0xC0000000)
	{
		data = .;
		*(.data)
	}

	.rodata : AT(ADDR(.rodata) - 0xC0000000)
	{
		rodata = .;
		*(.rodata)
	}

	.bss : AT(ADDR(.bss) - 0xC0000000)
	{
		bss = .;
		*(.bss)
	}
	end = .;
}
