#!/usr/bin/env sh

. $HOME/repo/lfs/_work_rpib_home/sdk/build/env.sh

LIBUSB_COMPAT_DIR=$HOME/repo/rpi_teensy/src/libusb/libusb-compat-0.1.4/install
LIBUSB_COMPAT_CFLAGS=-I$LIBUSB_COMPAT_DIR/include
LIBUSB_COMPAT_LFLAGS=-L$LIBUSB_COMPAT_DIR/lib

LIBUSB_DIR=$HOME/repo/rpi_teensy/src/libusb/libusb-1.0.9/install
LIBUSB_CFLAGS=
LIBUSB_LFLAGS=-L$LIBUSB_DIR/lib

$LFS_SDK_CROSS_COMPILE\gcc -Wall -O2 -DUSE_LIBUSB=1 \
$LIBUSB_COMPAT_CFLAGS $LIBUSB_CFLAGS \
$LIBUSB_COMPAT_LFLAGS $LIBUSB_LFLAGS \
load_linux_only.c -lusb -lusb-1.0 -lpthread -lrt
