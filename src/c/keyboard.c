#include <keyboard.h>

static char key_map[256] =
{
	0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
	'9', '0', '-', '=', '\b',	/* Backspace */
	'\t',			/* Tab */
	'q', 'w', 'e', 'r',	/* 19 */
	't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
	0,			/* 29   - Control */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
	'\'', '`',   0,		/* Left shift */
	'\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
	'm', ',', '.', '/',   0,				/* Right shift */
	'*',
	0,	/* Alt */
	' ',	/* Space bar */
	0,	/* Caps lock */
	0,	/* 59 - F1 key ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - Num lock*/
	0,	/* Scroll Lock */
	0,	/* Home key */
	0,	/* Up Arrow */
	0,	/* Page Up */
	'-',
	0,	/* Left Arrow */
	0,
	0,	/* Right Arrow */
	'+',
	0,	/* 79 - End key*/
	0,	/* Down Arrow */
	0,	/* Page Down */
	0,	/* Insert Key */
	0,	/* Delete Key */
	0,   0,   0,
	0,	/* F11 Key */
	0,	/* F12 Key */
	0,	/* All other keys are undefined */
	/* 90 through 128 undefined */
/* SHIFT VALUES add 90 to get to shift value */
	0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
	'(', ')', '_', '+', '\b',	/* BACKSPACE */
	'\t',			/* TAB */
	'Q', 'W', 'E', 'R',	/* 19 */
	'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* ENTER KEY */
	0,			/* 29   - CONTROL */
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
	'"', '~',   0,		/* LEFT SHIFT */
	'|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
	'M', '<', '>', '?',   0,			/* RIGHT SHIFT */
	'*',
	0,	/* ALT */
	' ',	/* SPACE BAR */
	0,	/* CAPS LOCK */
	0,	/* 59 - F1 KEY ... > */
	0,   0,   0,   0,   0,   0,   0,   0,
	0,	/* < ... F10 */
	0,	/* 69 - NUM LOCK*/
	0,	/* SCROLL LOCK */
	0,	/* HOME KEY */
	0,	/* UP ARROW */
	0,	/* PAGE UP */
	'-',
	0,	/* LEFT ARROW */
	0,
	0,	/* RIGHT ARROW */
	'+',
	0,	/* 79 - END KEY*/
	0,	/* DOWN ARROW */
	0,	/* PAGE DOWN */
	0,	/* INSERT KEY */
	0,	/* DELETE KEY */
	0,   0,   0,
	0,	/* F11 KEY */
	0,	/* F12 KEY */
	0,	/* ALL OTHER KEYS ARE UNDEFINED */
};

// keyboard buffer
static u8int keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static u16int buffer_length = 0; // buffer size is 4096 bytes, which can be expressed w/ 13 bits, so we can use a variable of 16 bits to keep track of the buffer size.
static void (*keyboard_handler)(u8int *buf, u16int size) = NULL; // this is a function that lives in the kernel which actually takes care of what to do w/ the input i recieve

void keyboard_set_handler(void (*callback)(u8int *buf, u16int size))
{
	keyboard_handler = callback;
}

// this is what disables the interrupts and calls the function in kernel.c to put the buffered data on the screen
void keyboard_flush()
{
	if (buffer_length > 0)
	{
		disable_interrupts();
		if (keyboard_handler != NULL)
		{
			keyboard_handler(keyboard_buffer, buffer_length);
		}
		buffer_length = 0;
		enable_interrupts();
	}
}

void keyboard_initialize()
{
	register_interrupt_handler(IRQ1, &keyboard_interrupt_handler);
}

void keyboard_interrupt_handler(__attribute__ ((unused)) registers regs)
{
	u8int scancode;
	
	scancode = inb(0x60);
	
	if (scancode & 0x80)
	{
		
	}
	else
	{
		keyboard_buffer[buffer_length++] = key_map[scancode];
		if (buffer_length == KEYBOARD_BUFFER_SIZE)
		{
			keyboard_flush();
		}
	}
}

