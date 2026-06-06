# Poggi OS - Makefile cu partiție FAT32 separată și injectată
SHELL = /bin/bash

CC      = gcc
LD      = ld
NASM    = nasm
PYTHON  = python3
QEMU    = qemu-system-x86_64

CFLAGS = -m64 -ffreestanding -mno-red-zone -mno-sse -nostdlib -fno-stack-protector -Iinclude -Wall -Wextra -O2 -DDEBUG
LDFLAGS = -m elf_x86_64 -T kernel/linker.ld -nostdlib -static
NASM_OBJ_FLAGS = -f elf64
NASM_BIN_FLAGS = -f bin

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

C_SOURCES = kernel/kernel.c kernel/task.c kernel/string.c drivers/vga.c drivers/keyboard.c drivers/heap.c drivers/ata.c drivers/serial.c drivers/fat32.c
ASM_SOURCES = kernel/entry.asm kernel/task_switch.asm

C_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(C_SOURCES)))
ASM_OBJS = $(patsubst %.asm, $(OBJ_DIR)/%.o, $(notdir $(ASM_SOURCES)))
KERNEL_OBJS = $(ASM_OBJS) $(C_OBJS)

BOOT_BIN = boot/boot.bin
KERNEL_BIN = $(BIN_DIR)/kernel.bin
DISK_IMAGE = poggi_disk.img
FAT32_PARTITION = fat32_partition.img

.PHONY: all clean run run-debug help

all: $(DISK_IMAGE)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR)/%.o: kernel/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: drivers/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: kernel/%.asm | $(OBJ_DIR)
	$(NASM) $(NASM_OBJ_FLAGS) $< -o $@

$(BOOT_BIN): boot/boot.asm
	$(NASM) $(NASM_BIN_FLAGS) $< -o $@

$(KERNEL_BIN): $(KERNEL_OBJS) | $(BIN_DIR)
	$(LD) $(LDFLAGS) -o $@ $^

$(FAT32_PARTITION): autorun.txt
	dd if=/dev/zero of=$@ bs=1M count=10 status=none
	mformat -i $@ -F -v "POGGIDATA" ::
	mcopy -i $@ autorun.txt ::

autorun.txt:
	echo "echo Salut din autorun!" > autorun.txt
	echo "poggi" >> autorun.txt

$(DISK_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) $(FAT32_PARTITION)
	dd if=/dev/zero of=$@ bs=1M count=32 status=none
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	# Injectează partiția FAT32 la offset 1MB (2048 sectoare)
	dd if=$(FAT32_PARTITION) of=$@ bs=512 seek=2048 conv=notrunc status=none
	# Populează PoggiFS (sectoarele 55-56 etc.)
	@if [ -d fs_root ]; then \
		echo "Populating PoggiFS from fs_root/"; \
		$(PYTHON) tools/poggi_mkfs.py --image $@ --root fs_root; \
	else \
		echo "Creating minimal test file for PoggiFS"; \
		echo "Test file content" > nota.txt; \
		dd if=nota.txt of=$@ bs=512 seek=56 conv=notrunc status=none 2>/dev/null; \
		$(PYTHON) -c 'import struct; n=b"nota.txt".ljust(16,b"\x00");lba=struct.pack("<I",56);sz=struct.pack("<I",19);open("idx.bin","wb").write(n+lba+sz+b"\x00"*8+b"\x00"*448)'; \
		dd if=idx.bin of=$@ bs=512 seek=55 conv=notrunc status=none; \
		rm -f nota.txt idx.bin; \
	fi

run: $(DISK_IMAGE)
	$(QEMU) -cpu qemu64 -drive format=raw,file=$(DISK_IMAGE)

run-debug: $(DISK_IMAGE)
	$(QEMU) -cpu qemu64 -drive format=raw,file=$(DISK_IMAGE) -serial stdio

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BOOT_BIN) $(DISK_IMAGE) $(FAT32_PARTITION) autorun.txt

help:
	@echo "Poggi OS - make | make run | make clean | make run-debug"