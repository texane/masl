#!/usr/bin/env sh

CC=$CROSS_COMPILE\gcc \
./configure --prefix=`pwd`/install --host=arm-linux
