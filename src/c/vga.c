#include <vga.h>

u16int *vga_mem = (u16int *) 0xC00B8000;
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

void put_hex(u32int n)
{
	put_str("0x\0");
	if (n == 0)
    {
        put_char('0');
        return;
    }
    u32int acc = n;
    u32int rem = 0;
    s32int i = 0;
    char hex[32];
    while (acc > 0)
    {
		rem = acc % 16;
		switch (rem)
		{
			case 10:
				hex[i] = 'A';
				break;
			case 11:
				hex[i] = 'B';
				break;
			case 12:
				hex[i] = 'C';
				break;
			case 13:
				hex[i] = 'D';
				break;
			case 14:
				hex[i] = 'E';
				break;
			case 15:
				hex[i] = 'F';
				break;
			default:
				hex[i] = '0' + rem;
				break;
		}
		i++;
		acc /= 16;
	}
	hex[i] = 0;
	char hex2[32];
    hex2[i--] = 0;
    int j = 0;
    while(i >= 0)
    {
        hex2[i--] = hex[j++];
    }
    put_str(hex2);
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

void clear_line()
{
	//index_ptr = vga_mem + (csr_y * scrn_width + csr_x);
	u16int blank_char = 0x20 | (attrib << 8);
	
	for (int i = 0; i < scrn_width; i++)
	{
		vga_mem[csr_y * scrn_width + i] = blank_char;
	}
	csr_x = 0;
	move_csr();
	
}

// setting up a buffer for the vga.
// anything wanting to output something to the screen should write its crap to this buffer
// the kernel will occationally flush the buffer to the screen.
// in this way i won't have crap crashing the system by trying to write to the screen while an interrupt is running

static u8int vga_buffer[VGA_BUFFER_SIZE];
static u16int vga_buffer_length = 0;
static void (*vga_handler)(u8int *buf, u16int size) = NULL;

// this sets the callback function on the kernel that actually takes care of business from kernel space
void vga_set_handler(void (*callback)(u8int *buf, u16int size))
{
	vga_handler = callback;
}

// calling this function will cause whwatever's in the buffer to be put on the screen, and clear the buffer
void vga_flush()
{
	if (vga_buffer_length > 0)
	{
		//disable_interrupts(); // i don't think writing to the screen should be an issue
		if (vga_handler != NULL)
		{
			vga_handler(vga_buffer, vga_buffer_length);
		}
		vga_buffer_length = 0;
		//enable_interrupts();
	}
}

void vga_buffer_put_char(char c)
{
	vga_buffer[vga_buffer_length++] = c;
	if (vga_buffer_length == VGA_BUFFER_SIZE)
	{
		vga_flush();
	}
}

void vga_buffer_put_str(char *str)
{
	int i = 0;
	while (str[i])
	{
		vga_buffer_put_char(str[i++]);
	}
}

void vga_buffer_put_dec(u32int n)
{
	if (n == 0)
    {
        vga_buffer_put_char('0');
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
    vga_buffer_put_str(c2);
}

void vga_buffer_put_hex(u32int n)
{
	vga_buffer_put_str("0x");
	if (n == 0)
    {
        vga_buffer_put_char('0');
        return;
    }
    u32int acc = n;
    u32int rem = 0;
    s32int i = 0;
    char hex[32];
    while (acc > 0)
    {
		rem = acc % 16;
		switch (rem)
		{
			case 10:
				hex[i] = 'A';
				break;
			case 11:
				hex[i] = 'B';
				break;
			case 12:
				hex[i] = 'C';
				break;
			case 13:
				hex[i] = 'D';
				break;
			case 14:
				hex[i] = 'E';
				break;
			case 15:
				hex[i] = 'F';
				break;
			default:
				hex[i] = '0' + rem;
				break;
		}
		i++;
		acc /= 16;
	}
	hex[i] = 0;
	char hex2[32];
    hex2[i--] = 0;
    int j = 0;
    while(i >= 0)
    {
        hex2[i--] = hex[j++];
    }
    vga_buffer_put_str(hex2);
}
