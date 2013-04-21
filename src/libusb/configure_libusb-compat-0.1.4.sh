#!/usr/bin/env sh

LIBUSB_1_0_LIBS=`pwd`/../libusb/libusb-1.0.9/install/lib/ \
CC=$CROSS_COMPILE\gcc \
./configure --prefix=`pwd`/install --host=arm-linux
