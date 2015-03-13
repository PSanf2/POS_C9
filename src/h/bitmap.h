#ifndef __BITMAP_H
#define __BITMAP_H

#include <system.h>

void bitmap_initialize(u32int frames);
void set_frame(u32int frame_addr);
void clear_frame(u32int frame_addr);
u32int test_frame(u32int frame_addr);
u32int first_free();

#endif
