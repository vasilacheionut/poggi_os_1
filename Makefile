# Poggi OS - Makefile final (fără HAL)
SHELL = /bin/bash

CC      = gcc
LD      = ld
NASM    = nasm
PYTHON  = python3
QEMU    = qemu-system-x86_64

CFLAGS = -m64 -ffreestanding -mno-red-zone -mno-sse -nostdlib -fno-stack-protector -Iinclude -Wall -Wextra -O2
LDFLAGS = -m elf_x86_64 -T kernel/linker.ld -nostdlib -static
NASM_OBJ_FLAGS = -f elf64
NASM_BIN_FLAGS = -f bin

BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Sursele tale (fără hal)
C_SOURCES = kernel/kernel.c kernel/task.c drivers/vga.c drivers/keyboard.c drivers/heap.c drivers/ata.c
ASM_SOURCES = kernel/entry.asm kernel/task_switch.asm

C_OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(notdir $(C_SOURCES)))
ASM_OBJS = $(patsubst %.asm, $(OBJ_DIR)/%.o, $(notdir $(ASM_SOURCES)))
KERNEL_OBJS = $(ASM_OBJS) $(C_OBJS)

BOOT_BIN = boot/boot.bin
KERNEL_BIN = $(BIN_DIR)/kernel.bin
OS_IMAGE = $(BIN_DIR)/os_image.bin

.PHONY: all clean run run-debug help

all: $(OS_IMAGE)

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

$(OS_IMAGE): $(BOOT_BIN) $(KERNEL_BIN) | $(BIN_DIR)
	dd if=/dev/zero of=$@ bs=512 count=81920 status=none
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc status=none
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=1 conv=notrunc status=none
	@if [ -d fs_root ]; then \
		echo "Populating PoggiFS from fs_root/"; \
		$(PYTHON) tools/poggi_mkfs.py --image $@ --root fs_root; \
	else \
		echo "Creating minimal test file"; \
		echo "Test file content" > nota.txt; \
		dd if=nota.txt of=$@ bs=512 seek=56 conv=notrunc status=none 2>/dev/null; \
		$(PYTHON) -c 'import struct; n=b"nota.txt".ljust(16,b"\x00");lba=struct.pack("<I",56);sz=struct.pack("<I",19);open("idx.bin","wb").write(n+lba+sz+b"\x00"*8+b"\x00"*448)'; \
		dd if=idx.bin of=$@ bs=512 seek=55 conv=notrunc status=none; \
		rm -f nota.txt idx.bin; \
	fi

run: $(OS_IMAGE)
	$(QEMU) -cpu qemu64 -drive format=raw,file=$(OS_IMAGE)

run-debug: $(OS_IMAGE)
	$(QEMU) -cpu qemu64 -drive format=raw,file=$(OS_IMAGE) -serial stdio

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BOOT_BIN)

help:
	@echo "Poggi OS - make | make run | make clean | make run-debug"