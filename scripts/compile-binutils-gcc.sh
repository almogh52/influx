#!/bin/bash

# Get the script's dir
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
UTILS_DIR=$DIR/../utils
UTILS_BIN_DIR=$UTILS_DIR/usr/local/bin
VENDOR_DIR=$DIR/../vendor
GCC_DIR=$VENDOR_DIR/gcc-influx
BINUTILS_DIR=$VENDOR_DIR/binutils-gdb-influx
BUILD_DIR=$DIR/../build
SYSROOT_DIR=$DIR/../sysroot

# Set compilers and linkers
GCC_PREFIX=$(brew --prefix gcc)
export CC=$GCC_PREFIX/bin/gcc-9
export CXX=$GCC_PREFIX/bin/g++-9
export CPP=$GCC_PREFIX/bin/cpp-9
export LD=$GCC_PREFIX/bin/gcc-9

# Add utils bin to path if not already in path
[[ ":$PATH:" != *":$UTILS_BIN_DIR:"* ]] && export PATH="$UTILS_BIN_DIR:$PATH"

# Configure binutils
echo -e '\033[0;33mConfiguring binutils..\033[0m'
mkdir -p $BUILD_DIR/binutils
cd $BUILD_DIR/binutils
$BINUTILS_DIR/configure --target=x86_64-influx --with-sysroot=$SYSROOT_DIR --with-build-sysroot=$SYSROOT_DIR --disable-nls --disable-werror --disable-gdb --disable-gdbsupport --disable-debug --disable-dependency-tracking --enable-deterministic-archives --enable-interwork --enable-multilib --enable-64-bit-bfd
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed configuring binutils, Exiting..\033[0m'
    exit 1
fi

# Compile binutils
echo -e '\033[0;33mCompiling binutils..\033[0m'
make
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed compiling binutils, Exiting..\033[0m'
    exit 1
fi

# Install binutils
echo -e '\033[0;33mInstalling binutils..\033[0m'
mkdir -p $UTILS_DIR
make DESTDIR=$UTILS_DIR install
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed installing binutils, Exiting..\033[0m'
    exit 1
fi

# Configure gcc
echo -e '\033[0;33mConfiguring gcc..\033[0m'
mkdir -p $BUILD_DIR/gcc
cd $BUILD_DIR/gcc
$GCC_DIR/configure --target=x86_64-influx --with-sysroot=$SYSROOT_DIR --with-build-sysroot=$SYSROOT_DIR --disable-nls --disable-werror --enable-languages=c,c++ --with-gnu-as --with-gnu-ld --with-gmp=$(brew --prefix gmp) --with-mpfr=$(brew --prefix mpfr) --with-mpc=$(brew --prefix libmpc) --with-isl=$(brew --prefix isl)
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed configuring gcc, Exiting..\033[0m'
    exit 1
fi

# Compile gcc
echo -e '\033[0;33mCompiling gcc..\033[0m'
make all-gcc
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed compiling gcc, Exiting..\033[0m'
    exit 1
fi

# Compile libgcc
echo -e '\033[0;33mCompiling libgcc..\033[0m'
make all-target-libgcc
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed compiling libgcc, Exiting..\033[0m'
    exit 1
fi

# Install gcc
echo -e '\033[0;33mInstalling gcc..\033[0m'
make DESTDIR=$UTILS_DIR install-gcc
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed installing gcc, Exiting..\033[0m'
    exit 1
fi

# Install libgcc
echo -e '\033[0;33mInstalling libgcc..\033[0m'
make DESTDIR=$UTILS_DIR install-target-libgcc
if [[ $? -ne 0 ]]; then
    echo -e '\033[0;31mFailed installing libgcc, Exiting..\033[0m'
    exit 1
fi

echo -e '\033[0;33mFinished compiling binutils and gcc.\033[0m'