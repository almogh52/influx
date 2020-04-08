#!/bin/bash

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Save the current dir
CURRNET_DIR=$(pwd)

# Create header for the font file
echo -e '\033[0;36mCreating header for font file..\033[0m'
cd $DIR/../vendor/scalable-font/fonts
xxd -i u_vga16.sfn > $DIR/../include/kernel/console/gfx_u_vga16_font.h
cd $CURRNET_DIR