#include <keyboard.h>

int shiftKeyDown;

// Keymaps: US International

// Non-Shifted scancodes to ASCII:
static char asciiNonShift[] = {
NULL, ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', BACKSPACE,
TAB, 'q', 'w',   'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',   '[', ']', ENTER, 0,
'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, 0, 0, ' ', 0,
KF1, KF2, KF3, KF4, KF5, KF6, KF7, KF8, KF9, KF10, 0, 0,
KHOME, KUP, KPGUP,'-', KLEFT, '5', KRIGHT, '+', KEND, KDOWN, KPGDN, KINS, KDEL, 0, 0, 0, KF11, KF12 };

// Shifted scancodes to ASCII:
static char asciiShift[] = {
NULL, ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', BACKSPACE,
TAB, 'Q', 'W',   'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P',   '{', '}', ENTER, 0,
'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~', 0, '|',
'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, 0, 0, ' ', 0,
KF1,   KF2, KF3, KF4, KF5, KF6, KF7, KF8, KF9, KF10, 0, 0,
KHOME, KUP, KPGUP, '-', KLEFT, '5',   KRIGHT, '+', KEND, KDOWN, KPGDN, KINS, KDEL, 0, 0, 0, KF11, KF12 };

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
	
	scancode = inb(0x60);	// get the scancode from the keyboard
	
	if (scancode & 0x80)	// was a key released? check bit 7 of scancode for this (10000000b = 0x80)
	{
		// compare only the low seven bits
		scancode &= 0x7F;
		if (scancode == KRLEFT_SHIFT || scancode == KRRIGHT_SHIFT)
		{
			shiftKeyDown = 0;
		}
	}
	else	// a key was pressed
	{
		// was the shift key pressed?
		if (scancode == KRLEFT_SHIFT || scancode == KRRIGHT_SHIFT)
		{
			shiftKeyDown = 1;
		}
		if (shiftKeyDown)
		{
			keyboard_buffer[buffer_length++] = asciiShift[scancode];
		}
		else
		{
			keyboard_buffer[buffer_length++] = asciiNonShift[scancode];
		}
		if (buffer_length == KEYBOARD_BUFFER_SIZE)
		{
			keyboard_flush();
		}
	}
}

