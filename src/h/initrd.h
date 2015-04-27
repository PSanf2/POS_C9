#ifndef __INITRD_H
#define __INITRD_H

#include <system.h>

typedef struct tar_header_struct
{
						/* offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
						/* 500 */
} tar_header_type;

void initrd_initialize(struct multiboot *mboot_ptr);
void print_tar_header(tar_header_type *tar_header);
void print_file_contents(tar_header_type *tar_header);
u32int get_size(const char *in);
u32int count_headers(u32int header_addr);

#endif
