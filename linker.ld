ENTRY(_start)

SECTIONS
{
	. = 1M;

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
		*(.rodata)
	}

	.bss : ALIGN(4K)
	{
		bss = .;
		*(.bss)
	}
	end = .;
}
