#include <vga.h>

u16int *vga_mem = (u16int *) 0xB8000;
u8int attrib = 0x0F;
u8int csr_x = 0;
u8int csr_y = 0;
u8int scrn_width = 80;
u8int scrn_height = 25;

void set_text_color(u8int foreground_color, u8int background_color)
{
	attrib = (background_color << 4) | (foreground_color & 0x0F);
}

void put_str(char *str)
{
	int i = 0;
	while (str[i])
	{
		put_char(str[i++]);
	}
}

void put_char(char c)
{
	u16int *index_ptr;
	u16int my_attrib = attrib << 8;
	
	if (c == 0x08) // backspace
	{
		if (csr_x != 0)
		{
			csr_x--;
		}
	}
	else if (c == 0x09) // tab
	{
		csr_x = (csr_x + 8) & ~(8 - 1); // increment x but only to a point that will make it divisible by 8 (i dunno, lawl)
	}
	else if (c == '\r') // carriage return, move cursor to beginning of row
	{
		csr_x = 0;
	}
	else if (c == '\n') // newline
	{
		csr_x = 0;
		csr_y++;
	}
	else if (c >= ' ') // any character greater than or equal to space is a printable character
	{
		// figure out where to put the character
		index_ptr = vga_mem + (csr_y * scrn_width + csr_x);
		// put the character and it's attribute into memory, which will put it on the screen when it refreshes.
		*index_ptr = c | my_attrib;
		csr_x++;
	}

	// make sure the cursor doesn't go off the right of the screen
	if (csr_x >= scrn_width) // if the cursor's off the screen
	{
		csr_x = 0; // move it to the left
		csr_y++; // and down
	}

	scroll();
	move_csr();
}

void scroll()
{
	u16int blank_char = 0x20 | (attrib << 8);
	
	if (csr_y >= scrn_height)
	{
		int i;
		for (i = 0; i < (scrn_height - 1) * scrn_width; i++)
		{
			vga_mem[i] = vga_mem[i+scrn_width];
		}
		
		for (i = (scrn_height - 1) * scrn_width; i < scrn_height * scrn_width; i++)
		{
			vga_mem[i] = blank_char;
		}
		
		csr_y = scrn_height - 1;
	}
}

void move_csr()
{
	u16int csr_loc = csr_y * scrn_width + csr_x;
	outb(0x3D4, 14);
	outb(0x3D5, csr_loc >> 8);
	outb(0x3D4, 15);
	outb(0x3D5, csr_loc);
}

void put_dec(u32int n)
{
    if (n == 0)
    {
        put_char('0');
        return;
    }

    s32int acc = n;
    char c[32];
    int i = 0;
    while (acc > 0)
    {
        c[i] = '0' + acc % 10;
        acc /= 10;
        i++;
    }
    c[i] = 0;

    char c2[32];
    c2[i--] = 0;
    int j = 0;
    while(i >= 0)
    {
        c2[i--] = c[j++];
    }
    put_str(c2);
}

void clear_screen()
{
	u16int blank_char = 0x20 | (attrib << 8);
	
	for (int i = 0; i < scrn_width * scrn_height; i++)
	{
		vga_mem[i] = blank_char;
	}
	
	csr_y = 0;
	move_csr();
}
