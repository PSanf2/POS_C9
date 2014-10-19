
# CONSTANTS

# Directories
TOOL_DIR = $$HOME/opt/cross/bin/
PROJECT_DIR = /media/sf_Operating_System_Stuff/POS\ Attempts/POS_C9/

# What to call the output
OUT_FILE_NAME = Patricks_OS

# Important programs
GAS = $(TOOL_DIR)i686-elf-as
GCC = $(TOOL_DIR)i686-elf-gcc
OBJCOPY = $(TOOL_DIR)i686-elf-objcopy

# Important files
LINKER_SCRIPT = linker.ld
GRUB_CFG = grub.cfg

# Flags
COMPILER_FLAGS = -c -ffreestanding -O2 -Wall -Wextra -std=gnu99 -ggdb -isystem src/h
LINKER_FLAGS = -T $(LINKER_SCRIPT) -ffreestanding -O2 -nostdlib -lgcc -ggdb

########################################################################

# MAKE RULES

GAS_FILES := $(wildcard src/as/*.as)
GAS_OBJ_FILES := $(addprefix build/, $(notdir $(GAS_FILES:.as=.as.o)))

build/%.as.o: src/as/%.as
	$(GAS) -o $@ $<

assemble: $(GAS_OBJ_FILES)

C_FILES := $(wildcard src/c/*.c)
C_OBJ_FILES := $(addprefix build/, $(notdir $(C_FILES:.c=.c.o)))

build/%.c.o: src/c/%.c
	$(GCC) $(COMPILER_FLAGS) -o $@ $<

compile: $(C_OBJ_FILES)

link: $(GAS_OBJ_FILES) $(C_OBJ_FILES)
	$(GCC) $(LINKER_FLAGS) -o build/$(OUT_FILE_NAME).bin $^

########################################################################

grub-iso: link
	mkdir -p build/isodir
	mkdir -p build/isodir/boot
	cp build/$(OUT_FILE_NAME).bin build/isodir/boot/$(OUT_FILE_NAME).bin
	mkdir -p build/isodir/boot/grub
	cp $(GRUB_CFG) build/isodir/boot/grub/$(GRUB_CFG)
	grub-mkrescue -o $(OUT_FILE_NAME).iso build/isodir

qemu-run: grub-iso
	qemu-system-i386 -cdrom $(OUT_FILE_NAME).iso -m 128M -monitor stdio
	
debug-run: grub-iso
	qemu-system-i386 -S -s -cdrom $(OUT_FILE_NAME).iso -monitor stdio

clean:
	rm -rf build/*.o
	rm -rf build/*.bin
	rm -rf build/isodir
