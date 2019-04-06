#!/bin/bash

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Build the iso
$DIR/create-bootstrap-iso.sh

# Run qemu with the iso
qemu-system-x86_64 -cdrom bootstrap.iso