# Functions
find_includes_in_dir    = $(find $(1) -name "*.h" | sed 's|/[^/]*$$||' | sort -u)

# Toolchain Configuration
CC                      := clang
C_STANDARD              := -std=gnu14
PREFIX                  := /usr/local/opt/llvm/bin

# C Compiler -- Flags
CFLAGS                  += -masm=intel
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

# Linker flags
LDFLAGS                 += -nostdlib
LDFLAGS                 += -O2

# Build Configuration
OBJ_DIR                 := obj
SYSROOT_DIR             := sysroot
EFI_DIR                 := EFI

BOOT_DIR                := boot
BOOT_SRC_FILES          := $(wildcard ${BOOT_DIR}/*.s) $(wildcard ${BOOT_DIR}/*.c)
BOOT_OBJ_FILES          := $(addprefix $(OBJ_DIR)/, $(BOOT_SRC_FILES))
BOOT_OBJ_FILES          := $(BOOT_OBJ_FILES:.s=.o)
BOOT_OBJ_FILES          := $(BOOT_OBJ_FILES:.c=.o)

.PHONY: all clean

all: $(EFI_DIR)/$(BOOT_DIR)/BOOTX64.EFI

clean:
	rm -rf $(SYSROOT_DIR) $(OBJ_DIR) $(EFI_DIR)

$(EFI_DIR)/$(BOOT_DIR)/BOOTX64.EFI: $(BOOT_OBJ_FILES)
	@echo 'Compiling EFI boot executable'
	@mkdir -p $(@D)
	$(PREFIX)/ld.lld -Tboot/linker.ld $(LDFLAGS) $< -o $@

$(OBJ_DIR)/%.o: %.s
	@echo 'Compiling $<'
	@mkdir -p $(@D)
	nasm -f elf64 $< -o $@

$(OBJ_DIR)/%.o: %.c
	@echo 'Compiling $<'
	@mkdir -p $(@D)
	$(PREFIX)/$(CC) $(CFLAGS) -c $< -o $@