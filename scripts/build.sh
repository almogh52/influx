#!/bin/bash

OS_NAME=influx

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Generate headers
echo -e '\033[0;36mGenerating headers..\033[0m'
$DIR/generate_headers.sh

echo -e '\033[0;36mCompiling..\033[0m'

# Build the project if it's not built
if [[ $1 == "DEBUG" ]]; then
    make DEBUG=1
else
    make
fi

# If the build failed
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mBuild failed, Exiting..\033[0m'
    exit 1
fi

# Create the isodir and it's sub-dirs
echo -e '\033[0;36mCreating the iso directories..\033[0m'
mkdir -p iso/boot

# Copy the bootstrap executable from the build dir
echo -e '\033[0;36mCopying bootstrap executable..\033[0m'
cp build/kernel/EFI/boot/$OS_NAME-bootstrap.bin iso/boot/$OS_NAME-bootstrap.bin

# Copy the kernel executable from the build dir
echo -e '\033[0;36mCopying kernel executable..\033[0m'
cp build/kernel/EFI/boot/$OS_NAME-kernel.bin iso/boot/$OS_NAME-kernel.bin
x86_64-elf-strip --strip-unneeded iso/boot/$OS_NAME-kernel.bin

# Copy the grub dir
echo -e '\033[0;36mCopying grub configuration folder..\033[0m'
cp -a grub iso/boot

echo -e '\033[0;36mMaking iso from the iso dir..\033[0m'
grub-mkrescue -o influx.iso iso

# Create sysroot if not exists
mkdir -p sysroot

# Check for mtree changes
echo -e '\033[0;36mChecking mtree changes..\033[0m'
mtree -p sysroot < sysroot.mtree >/dev/null 2>&1
if [[ $? -ne 0 ]]; then
    # Regenerate mtree
    mtree -c -K sha256digest -p sysroot > sysroot.mtree

    # Create hard drive image
    echo -e '\033[0;36mMaking drive image from sysroot..\033[0m'
    rm -f hdd.img
    $(brew --prefix e2fsprogs)/sbin/mke2fs -d sysroot -t ext2 hdd.img 512M
fi