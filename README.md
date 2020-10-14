# Influx

Influx is a x86_64 operation system based on a monolithic kernel.

![Kernel - ttys1](https://i.imgur.com/0D6ycaW.png)

## Features

* 32-bit bootstrap bootable using any multiboot-compliant bootloader (GRUB used)
  * Protected mode (32-bit) to long mode (64-bit) + Setup of basic 64-bit paging
  * SSE enable
  * x86_64 ELF parser and loader (used for kernel executable)
* Multithreaded x86_64 kernel loaded by the bootstrap
  * Memory Manager
  * Driver Manager
  * ATA, PCI, CMOS, PIT, BGA graphics, PS2-Keyboard Drivers
  * Scheduler based round-robin scheduling (Full support for userland processes and kernel threads)
  * Graphics console based on BGA graphics
  * Interrupt and IRQ manager
  * TTY manager
  * VFS (with support for pipes)
  * Read-write ext2 filesystem
  * x86_64 ELF parser
  * 35 syscalls conforming to Linux and POSIX specification
  * Newlib C library with support for 35 syscalls
  * Init process used to start userland processes
  * Support for creating 64-bit userland processes including arguments and environment variables
  * Bash support (WIP)

## Build

### Installing toolchain

#### macOS (using Homebrew)

```bash
brew install almogh52/x86_64-elf-toolchain/x86_64-elf-gcc gcc qemu nasm
```

Compiling grub for macOS is available [here](https://wiki.osdev.org/GRUB#Installing_GRUB_2_on_OS_X). (Compile for target x86_64-elf)

### Building newlib, binutils and gcc for influx

```bash
./scripts/compile-binutils-gcc.sh
./scripts/compile-newlib.sh
```

### Building influx for release and running it

```bash
./scripts/run-qemu.sh
```

### Cleaning build

```bash
./scripts/clean.sh
```

## Debugging

### Debugging bootstrap

```bash
./scripts/debug-bootstrap.sh
```

### Debugging kernel

```bash
./scripts/debug-kernel.sh
```
