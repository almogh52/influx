#!/bin/bash

OS_NAME=myos

# Build the project if it's not built
make

# If the build failed
if [ $? -ne 0 ]; then
    echo 'Build failed, Exiting..'
    exit
fi

# Create the isodir and it's sub-dirs
echo 'Creating the iso directories.'
mkdir -p iso/boot iso/boot/grub

# Copy the grub configuration file to it
echo 'Copying grub configuration file.'
cp boot/grub.cfg iso/boot/grub

# Copy the bootstrap executable from the build dir
echo 'Copying bootstrap executable.'
cp build/EFI/boot/$OS_NAME-bootstrap.bin iso/boot/$OS_NAME-bootstrap.bin

echo 'Making iso from the iso dir'
grub-mkrescue -o bootstrap.iso iso