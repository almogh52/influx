# Functions
find_includes_in_dir    = $(find $(1) -name "*.h" | sed 's|/[^/]*$$||' | sort -u)

# OS Configuration
HOST_OS_NAME := $(shell uname -s | tr A-Z a-z)
OS_NAME                 := influx

# Toolchain Configuration
CC                      := clang
AS                      := nasm
LINK                    := ld.lld
C_STANDARD              := -std=gnu18
ifeq (${HOST_OS_NAME},darwin)
	PREFIX              := /usr/local/opt/llvm/bin
else
	PREFIX              := /usr/bin
endif

# Assembler -- Flags
ASFLAGS                 += -f elf64

# C Compiler -- Flags
CFLAGS                  += $(C_STANDARD)
CFLAGS                  += --target=x86_64-unknown-none-elf
CFLAGS                  += -ffreestanding
CFLAGS                  += -fshort-wchar
CFLAGS                  += -mno-red-zone

# C Compiler -- Warnings 
CFLAGS                  += $(addprefix -I, $(INC_DIRS))
CFLAGS                  += -Wall
CFLAGS                  += -Wextra
CFLAGS                  += -Wfatal-errors
CFLAGS                  += -Wpacked
CFLAGS                  += -Winline
CFLAGS                  += -Wfloat-equal
CFLAGS                  += -Wconversion
CFLAGS                  += -Wpointer-arith
CFLAGS                  += -Wdisabled-optimization
CFLAGS                  += -Wno-unused-parameter
CFLAGS                  += -Qunused-arguments

# If debug mode set, generate symbols from source files
ifdef DEBUG
	ASFLAGS             += -F dwarf -g
	CFLAGS              += -g
	LDFLAGS             += -g
endif

# Linker flags
LDFLAGS                 += -nostdlib

# Build Configuration
BUILD_DIR               := build
OBJ_DIR                 := $(BUILD_DIR)/obj
SYSROOT_DIR             := $(BUILD_DIR)/sysroot
EFI_DIR                 := $(BUILD_DIR)/EFI

BOOT_DIR                := boot
BOOT_SRC_FILES          := $(wildcard ${BOOT_DIR}/*.s) $(wildcard ${BOOT_DIR}/*.c)
BOOT_INC_DIR            := ${BOOT_DIR}/include
BOOT_OBJ_FILES          := $(addprefix $(OBJ_DIR)/, $(BOOT_SRC_FILES))
BOOT_OBJ_FILES          := $(BOOT_OBJ_FILES:.s=.o)
BOOT_OBJ_FILES          := $(BOOT_OBJ_FILES:.c=.o)

KERNEL_DIR              := kernel
KERNEL_SRC_FILES        := $(wildcard ${KERNEL_DIR}/*.s) $(wildcard ${KERNEL_DIR}/*.c)
KERNEL_OBJ_FILES        := $(addprefix $(OBJ_DIR)/, $(KERNEL_SRC_FILES))
KERNEL_OBJ_FILES        := $(KERNEL_OBJ_FILES:.s=.o)
KERNEL_OBJ_FILES        := $(KERNEL_OBJ_FILES:.c=.o)

.PHONY: all clean debug

all: $(EFI_DIR)/$(BOOT_DIR)/$(OS_NAME)-bootstrap.bin $(EFI_DIR)/$(BOOT_DIR)/$(OS_NAME)-kernel.bin

clean:
	rm -rf $(BUILD_DIR)

$(EFI_DIR)/$(BOOT_DIR)/$(OS_NAME)-bootstrap.bin: $(BOOT_OBJ_FILES)
	@echo 'Linking bootstrap executable'
	@mkdir -p $(@D)
	$(PREFIX)/$(LINK) -Tboot/linker.ld $(LDFLAGS) $(BOOT_OBJ_FILES) -o $@

$(OBJ_DIR)/$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.s
	@echo 'Compiling $<'
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.c
	@echo 'Compiling $<'
	@mkdir -p $(@D)
	$(PREFIX)/$(CC) -I${BOOT_INC_DIR} $(CFLAGS) -c $< -o $@

$(EFI_DIR)/$(BOOT_DIR)/$(OS_NAME)-kernel.bin: ${KERNEL_OBJ_FILES}
	@echo 'Linking kernel executable'
	@mkdir -p $(@D)
	$(PREFIX)/$(LINK) $(LDFLAGS) $(KERNEL_OBJ_FILES) -o $@

$(OBJ_DIR)/${KERNEL_DIR}/%.o: ${KERNEL_DIR}/%.s
	@echo 'Compiling $<'
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/${KERNEL_DIR}/%.o: ${KERNEL_DIR}/%.c
	@echo 'Compiling $<'
	@mkdir -p $(@D)
	$(PREFIX)/$(CC) $(CFLAGS) -c $< -o $@