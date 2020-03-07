#!/bin/bash

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Build the iso
$DIR/create-iso.sh

# If the build failed
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mCreating ISO failed, Exiting..\033[0m'
    exit 1
fi

# Create hard drive image
if [[ ! -f "hdd.img" ]]; then
    qemu-img create hdd.img 512M
fi

# Run qemu with the iso
echo -e '\033[0;36mRunning QEMU..\033[0m'
qemu-system-x86_64 -boot d -cdrom influx.iso -m 4G -drive file=hdd.img,format=raw