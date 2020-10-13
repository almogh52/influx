#!/bin/bash

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
UTILS_DIR=$DIR/../utils
UTILS_BIN_DIR=$UTILS_DIR/usr/local/bin
VENDOR_DIR=$DIR/../vendor
NEWLIB_DIR=$VENDOR_DIR/newlib-influx
BUILD_DIR=$DIR/../build
SYSROOT_DIR=$DIR/../sysroot

# Add utils bin to path if not already in path
[[ ":$PATH:" != *":$UTILS_BIN_DIR:"* ]] && export PATH="$UTILS_BIN_DIR:$PATH"

# Check if x86_64-influx-gcc exists
if [ ! -f $UTILS_BIN_DIR/x86_64-influx-gcc ]; then
	# Hacky trick to get x86_64-influx-gcc
	HACKY_GCC=1
	GCC_PREFIX=$(brew --prefix x86_64-elf-gcc)
	BINUTILS_PREFIX=$(brew --prefix x86_64-elf-binutils)
	mkdir -p $UTILS_DIR
	mkdir -p $UTILS_BIN_DIR
	ln $BINUTILS_PREFIX/bin/x86_64-elf-ar $UTILS_BIN_DIR/x86_64-influx-ar
	ln $BINUTILS_PREFIX/bin/x86_64-elf-as $UTILS_BIN_DIR/x86_64-influx-as
	ln $GCC_PREFIX/bin/x86_64-elf-gcc $UTILS_BIN_DIR/x86_64-influx-gcc
	ln $GCC_PREFIX/bin/x86_64-elf-gcc $UTILS_BIN_DIR/x86_64-influx-cc
	ln $BINUTILS_PREFIX/bin/x86_64-elf-ranlib $UTILS_BIN_DIR/x86_64-influx-ranlib
	mkdir -p $UTILS_DIR/usr/local/lib
	ln -s $GCC_PREFIX/lib/gcc $UTILS_DIR/usr/local/lib/gcc
	mkdir -p $UTILS_DIR/usr/local/libexec
	ln -s $GCC_PREFIX/libexec/gcc $UTILS_DIR/usr/local/libexec/gcc
	mkdir -p $UTILS_DIR/usr/local/include
	ln -s $GCC_PREFIX/include/c++ $UTILS_DIR/usr/local/include/c++
fi

# Configure newlib
echo -e '\033[0;33mConfiguring newlib..\033[0m'
mkdir -p $BUILD_DIR/newlib
cd $BUILD_DIR/newlib
$NEWLIB_DIR/configure --prefix=$SYSROOT_DIR/usr --target=x86_64-influx
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed configuring newlib, Exiting..\033[0m'
    exit 1
fi

# Compile newlib
echo -e '\033[0;33mCompiling newlib..\033[0m'
make all
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed compiling newlib, Exiting..\033[0m'
    exit 1
fi

# Install newlib
echo -e '\033[0;33mInstalling newlib..\033[0m'
mkdir -p $SYSROOT_DIR/usr
make install
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed installing newlib, Exiting..\033[0m'
    exit 1
fi

# Copy the contents of the install to the correct usr directory
cp -r $SYSROOT_DIR/usr/x86_64-influx/* $SYSROOT_DIR/usr
rm -rf $SYSROOT_DIR/usr/x86_64-influx

# Remove hacky GCC
if [ "$HACKY_GCC" -eq "1" ]; then
	rm -f $UTILS_BIN_DIR/x86_64-influx-ar
	rm -f $UTILS_BIN_DIR/x86_64-influx-as
	rm -f $UTILS_BIN_DIR/x86_64-influx-gcc
	rm -f $UTILS_BIN_DIR/x86_64-influx-cc
	rm -f $UTILS_BIN_DIR/x86_64-influx-ranlib
	rm -f $UTILS_DIR/usr/local/lib/gcc
	rm -f $UTILS_DIR/usr/local/libexec/gcc
	rm -f $UTILS_DIR/usr/local/include/c++
fi

echo -e '\033[0;33mFinished compiling newlib.\033[0m'