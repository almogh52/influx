# Functions
rwildcard               = $(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))

# OS Configuration
HOST_OS_NAME            := $(shell uname -s | tr A-Z a-z)
OS_NAME                 := influx

# Color configuration
COM_COLOR               := \033[0;32m
LNK_COLOR               := \033[0;33m
OBJ_COLOR               := \033[0;35m
NO_COLOR                := \033[m

# Toolchain Configuration
CC                      := x86_64-elf-gcc
CXX                     := x86_64-elf-g++
AS                      := nasm
LINK                    := x86_64-elf-ld
C_STANDARD              := -std=gnu18
CXX_STANDARD            := -std=c++17
ifeq (${HOST_OS_NAME},darwin)
	PREFIX              := /usr/local/bin
else
	PREFIX              := /usr/bin
endif

# Build Configuration - Include
INCLUDE_DIR             := include vendor/scalable-font

# Assembler -- Flags
ASFLAGS                 += -f elf64
ASFLAGS                 += -i kernel

# C Compiler -- Flags
CFLAGS                  += $(addprefix -I, $(INCLUDE_DIR))
CFLAGS                  += -O3
CFLAGS                  += -masm=intel
CFLAGS                  += -mcmodel=large
CFLAGS                  += -ffreestanding
CFLAGS                  += -fshort-wchar
CFLAGS                  += -mno-red-zone
CFLAGS                  += -mno-avx

# C Compiler -- Warnings 
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

# If debug mode set, generate symbols from source files
ifdef DEBUG
	ASFLAGS             += -F dwarf -g
	CFLAGS              += -g
	LDFLAGS             += -g
endif

# C++ Compiler -- Flags
CXXFLAGS                := ${CFLAGS}
CXXFLAGS                += -fno-exceptions
CXXFLAGS                += -fno-rtti

# Set compile standards
CFLAGS                  += $(C_STANDARD)
CXXFLAGS                += $(CXX_STANDARD)

# libgcc location
LIBGCC_DIR              := $(dir $(shell $(CC) $(CFLAGS) -print-libgcc-file-name))

# Linker flags
LDFLAGS                 += -nostdlib
LDFLAGS                 += -z max-page-size=0x1000
LDFLAGS                 += -L${LIBGCC_DIR}
LDFLAGS                 += -lgcc

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
KERNEL_CRTI_OBJ         := ${OBJ_DIR}/${KERNEL_DIR}/crti.o
KENREL_CRTBEGIN_OBJ     := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o)
KERNEL_CRTEND_OBJ       := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o)
KERNEL_CRTN_OBJ         := ${OBJ_DIR}/${KERNEL_DIR}/crtn.o
KERNEL_SRC_FILES        := $(call rwildcard,${KERNEL_DIR},*.s *.c *.cpp)
KERNEL_OBJ_FILES        := $(addprefix $(OBJ_DIR)/, $(KERNEL_SRC_FILES))
KERNEL_OBJ_FILES        := $(KERNEL_OBJ_FILES:.s=.o)
KERNEL_OBJ_FILES        := $(KERNEL_OBJ_FILES:.c=.o)
KERNEL_OBJ_FILES        := $(KERNEL_OBJ_FILES:.cpp=.o)
KERNEL_OBJ_FILES        := $(filter-out ${KERNEL_CRTI_OBJ} ${KERNEL_CRTN_OBJ}, ${KERNEL_OBJ_FILES})
KERNEL_OBJ_FILES        := $(KERNEL_CRTI_OBJ) $(KENREL_CRTBEGIN_OBJ) ${KERNEL_OBJ_FILES} $(KERNEL_CRTEND_OBJ) $(KERNEL_CRTN_OBJ)

.PHONY: all clean debug

all: $(EFI_DIR)/$(BOOT_DIR)/$(OS_NAME)-bootstrap.bin $(EFI_DIR)/$(BOOT_DIR)/$(OS_NAME)-kernel.bin

clean:
	rm -rf $(BUILD_DIR)

$(EFI_DIR)/$(BOOT_DIR)/$(OS_NAME)-bootstrap.bin: $(BOOT_OBJ_FILES) ${BOOT_DIR}/linker.ld
	@printf '%b' '$(LNK_COLOR)Linking bootstrap executable$(NO_COLOR)\n'
	@mkdir -p $(@D)
	$(PREFIX)/$(LINK) -T${BOOT_DIR}/linker.ld $(LDFLAGS) $(BOOT_OBJ_FILES) -o $@

$(OBJ_DIR)/$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.s
	@printf '%b' '$(COM_COLOR)Compiling $(OBJ_COLOR)$<$(NO_COLOR)\n'
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/$(BOOT_DIR)/%.o: $(BOOT_DIR)/%.c
	@printf '%b' '$(COM_COLOR)Compiling $(OBJ_COLOR)$<$(NO_COLOR)\n'
	@mkdir -p $(@D)
	$(PREFIX)/$(CC) -I${BOOT_INC_DIR} $(CFLAGS) -c $< -o $@

$(EFI_DIR)/$(BOOT_DIR)/$(OS_NAME)-kernel.bin: ${KERNEL_OBJ_FILES} ${KERNEL_DIR}/linker.ld
	@printf '%b' '$(LNK_COLOR)Linking kernel executable$(NO_COLOR)\n'
	@echo ${LIBGCC_DIR}
	@mkdir -p $(@D)
	$(PREFIX)/$(LINK) -T${KERNEL_DIR}/linker.ld $(LDFLAGS) $(KERNEL_OBJ_FILES) -o $@

$(OBJ_DIR)/${KERNEL_DIR}/%.o: ${KERNEL_DIR}/%.s
	@printf '%b' '$(COM_COLOR)Compiling $(OBJ_COLOR)$<$(NO_COLOR)\n'
	@mkdir -p $(@D)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJ_DIR)/${KERNEL_DIR}/%.o: ${KERNEL_DIR}/%.c
	@printf '%b' '$(COM_COLOR)Compiling $(OBJ_COLOR)$<$(NO_COLOR)\n'
	@mkdir -p $(@D)
	$(PREFIX)/$(CC) $(CFLAGS) -c $< -o $@
	
$(OBJ_DIR)/${KERNEL_DIR}/%.o: ${KERNEL_DIR}/%.cpp
	@printf '%b' '$(COM_COLOR)Compiling $(OBJ_COLOR)$<$(NO_COLOR)\n'
	@mkdir -p $(@D)
	$(PREFIX)/$(CXX) $(CXXFLAGS) -c $< -o $@
