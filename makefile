# StartOS Makefile

CFLAGS = -mcmodel=kernel -m64 -ffreestanding -fno-stack-protector -Wall -Wextra -c -fno-pie -fno-pic -Wno-missing-braces
LDFLAGS = -m elf_x86_64 -T linker.ld
ASFLAGS = -f elf64

SRC_DIR = src
BUILD_DIR = build
ISO_DIR = iso

LIMINE_DIR = limine
LIMINE_BINARIES = $(LIMINE_DIR)/limine-bios.sys $(LIMINE_DIR)/limine-bios-cd.bin

C_SOURCES = $(shell find $(SRC_DIR) -name "*.c")
ASM_SOURCES = $(shell find $(SRC_DIR) -name "*.asm")

OBJ = $(C_SOURCES:.c=.o) $(ASM_SOURCES:.asm=.o)

KERNEL = $(BUILD_DIR)/kernel.bin

ISO_IMAGE = StartOS.iso

all: $(ISO_IMAGE)

%.o: %.c
	clang $(CFLAGS) $< -o $@

%.o: %.asm
	nasm $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJ)
	@mkdir -p $(BUILD_DIR)
	ld $(LDFLAGS) $(OBJ) -o $@


$(ISO_IMAGE): $(KERNEL) $(LIMINE_BINARIES)
	@mkdir -p $(ISO_DIR)/boot
	cp $(KERNEL) $(ISO_DIR)/boot/
	cp $(SRC_DIR)/boot/limine.conf $(ISO_DIR)/boot/
	cp $(LIMINE_BINARIES) $(ISO_DIR)/boot/
	xorriso -as mkisofs \
		-b boot/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		-o $(ISO_IMAGE) $(ISO_DIR)
	$(LIMINE_DIR)/limine bios-install $(ISO_IMAGE)

clean:
	rm -rf $(OBJ) $(KERNEL) $(ISO_IMAGE) $(ISO_DIR) $(BUILD_DIR)

run:
	virtualboxvm --startvm "StartOS" &

qemu:
	qemu-system-x86_64 -cdrom StartOS.iso -audiodev pa,id=snd0 -machine pcspk-audiodev=snd0 -m 1235M -drive file=/run/media/rishies2010/Storage/VMs/StartOS/StartOS.vhd,if=ide,index=0  -boot d &

stop:
	VBoxManage controlvm "StartOS" poweroff

out:
	socat - UNIX-CONNECT:/tmp/startos

gdb:
	gdb build/kernel.bin

.PHONY: all clean run qemu out stop gdb
