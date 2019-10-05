#!/bin/bash

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Clean the old build in case it's not built with debug symbols
$DIR/clean.sh

# Build the iso
$DIR/create-bootstrap-iso.sh DEBUG

# Run qemu with the iso
qemu-system-x86_64 -cdrom influx.iso -s -S &

# Run gdb
gdb -ex "file iso/boot/influx-bootstrap.bin" -ex "layout asm" -ex "target remote localhost:1234" -ex "break _start" -ex "continue"

# Close all qemu processes
killall qemu-system-x86_64