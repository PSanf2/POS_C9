#ifndef __VGA_H
#define __VGA_H

#include <system.h>

extern void set_text_color(u8int foreground_color, u8int background_color);
extern void put_str(char *str);
extern void put_char(char c);
extern void scroll();
extern void move_csr();
extern void put_dec(u32int n);
extern void clear_screen();

#endif
