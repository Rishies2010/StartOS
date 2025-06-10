# StartOS Makefile

CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -Wall -Wextra -c -Os
LDFLAGS = -m elf_i386 -T linker.ld
ASFLAGS = -f elf32

SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso

C_SOURCES = $(shell find $(SRC_DIR) -name "*.c")
ASM_SOURCES = $(shell find $(SRC_DIR) -name "*.asm")

OBJ = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o)

KERNEL = $(BUILD_DIR)/kernel.bin

ISO_IMAGE = StartOS.iso

all: $(ISO_IMAGE)

%.o: %.c
	i686-elf-gcc $(CFLAGS) $< -o $@

%.o: %.asm
	nasm $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	i686-elf-ld $(LDFLAGS) $(OBJ) -o $@

$(ISO_IMAGE): $(KERNEL)
	@mkdir -p $(ISO_DIR)/boot/grub
	cp $(KERNEL) $(ISO_DIR)/boot/
	cp src/boot/grub.cfg $(ISO_DIR)/boot/grub/
	grub2-mkrescue -o $(ISO_IMAGE) $(ISO_DIR)

clean:
	rm -rf $(OBJ) $(KERNEL) $(ISO_IMAGE) $(ISO_DIR)/boot/kernel.bin

run:
	virtualboxvm --startvm "StartOS" &

stop:
	VBoxManage controlvm "StartOS" poweroff

.PHONY: all clean run
