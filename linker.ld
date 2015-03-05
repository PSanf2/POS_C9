ENTRY(_start)

SECTIONS
{
	. = 1M;
	start = .;
	.text : ALIGN(4K)
	{
		*(.multiboot)
		code = .;
		*(.text)
	}

	.data : ALIGN(4K)
	{
		data = .;
		*(.data)
	}

	.rodata : ALIGN(4K)
	{
		rodata = .;
		*(.rodata)
	}

	.bss : ALIGN(4K)
	{
		bss = .;
		*(.bss)
	}
	end = .;
}
