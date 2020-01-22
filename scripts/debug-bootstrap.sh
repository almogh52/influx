#!/bin/bash

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Clean the old build in case it's not built with debug symbols
$DIR/clean.sh

# Build the iso
$DIR/create-iso.sh DEBUG

# If the build failed
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mCreating ISO failed, Exiting..\033[0m'
    exit 1
fi

# Run qemu with the iso
echo -e '\033[0;36mRunning QEMU and starting debugging..\033[0m'
qemu-system-x86_64 -cdrom influx.iso -s -S &

# Run gdb
gdb -ex "file iso/boot/influx-bootstrap.bin" -ex "layout asm" -ex "target remote localhost:1234"

# Close all qemu processes
killall qemu-system-x86_64 > /dev/null 2>&1i