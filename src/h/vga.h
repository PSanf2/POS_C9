#ifndef __VGA_H
#define __VGA_H

#include <system.h>

#define BLACK 0
#define BLUE 1
#define GREEN 2
#define CYAN 3
#define RED 4
#define MAGENTA 5
#define BROWN 6
#define LIGHT_GREY 7
#define DARK_GREY 8
#define LIGHT_BLUE 9
#define LIGHT_GREEN 10
#define LIGHT_CYAN 11
#define LIGHT_RED 12
#define LIGHT_MAGENTA 13
#define LIGHT_BROWN 14
#define WHITE 15

#define VGA_BUFFER_SIZE 4096

extern void set_text_color(u8int foreground_color, u8int background_color);
extern void put_str(char *str);
extern void put_char(char c);
extern void scroll();
extern void move_csr();
extern void put_dec(u32int n);
extern void clear_screen();
extern void vga_set_handler(void (*callback)(u8int *buf, u16int size));
extern void vga_flush();
extern void vga_buffer_put_char(char c);
extern void vga_buffer_put_str(char *str);

#endif
