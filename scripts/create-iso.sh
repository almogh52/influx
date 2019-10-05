#!/bin/bash

OS_NAME=influx

# Build the project if it's not built
if [[ "$1" -eq "DEBUG" ]]; then
    make DEBUG=1
else
    make
fi

# If the build failed
if [[ $? -ne 0 ]]; then
    echo 'Build failed, Exiting..'
    exit 1
fi

# Create the isodir and it's sub-dirs
echo 'Creating the iso directories.'
mkdir -p iso/boot iso/boot/grub

# Copy the grub configuration file to it
echo 'Copying grub configuration file.'
cp grub.cfg iso/boot/grub

# Copy the bootstrap executable from the build dir
echo 'Copying bootstrap executable.'
cp build/EFI/boot/$OS_NAME-bootstrap.bin iso/boot/$OS_NAME-bootstrap.bin

# Copy the kernel executable from the build dir
echo 'Copying kernel executable.'
cp build/EFI/boot/$OS_NAME-kernel.bin iso/boot/$OS_NAME-kernel.bin

# Copy the grub dir
echo 'Copying grub configuration folder.'
cp -a grub iso/boot

echo 'Making iso from the iso dir'
grub-mkrescue -o influx.iso iso