#!/bin/bash

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Build the iso
$DIR/create-iso.sh

# If the build failed
if [[ $? -ne 0 ]]; then
    echo 'Creating ISO failed, Exiting..'
    exit 1
fi

# Run qemu with the iso
qemu-system-x86_64 -cdrom myos.iso